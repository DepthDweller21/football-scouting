import { useCallback, useEffect, useState } from 'react'
import { ApiError } from '../api/client'
import {
  addTeamPlayer,
  createTeam,
  deleteTeam,
  fetchTeam,
  fetchTeamAnalysis,
  removeTeamPlayer,
  updateTeam,
} from '../api/scouting'
import { useAuth } from '../context/AuthContext'
import { FORMATION_OPTIONS, PREFERRED_SPOTS, SEED_TEAM_IDS, TEAM_STORAGE_KEY } from '../constants'
import Alert from '../components/Alert'
import FormationLineup from '../components/FormationLineup'
import FormationSimulator from '../components/FormationSimulator'

function loadSavedTeamIds() {
  try {
    const raw = localStorage.getItem(TEAM_STORAGE_KEY)
    const parsed = raw ? JSON.parse(raw) : []
    const stored = Array.isArray(parsed) ? parsed.filter((id) => Number.isFinite(id)) : []
    return [...new Set([...SEED_TEAM_IDS, ...stored])]
  } catch {
    return [...SEED_TEAM_IDS]
  }
}

function saveTeamId(id) {
  const ids = loadSavedTeamIds()
  if (!ids.includes(id)) {
    localStorage.setItem(TEAM_STORAGE_KEY, JSON.stringify([...ids, id]))
  }
}

function removeSavedTeamId(id) {
  const ids = loadSavedTeamIds().filter((x) => x !== id)
  localStorage.setItem(TEAM_STORAGE_KEY, JSON.stringify(ids))
}

export default function TeamsPage() {
  const { user, canWrite } = useAuth()
  const [teamIds, setTeamIds] = useState(loadSavedTeamIds)
  const [selectedId, setSelectedId] = useState(teamIds[0] ?? null)
  const [team, setTeam] = useState(null)
  const [roster, setRoster] = useState([])
  const [analysis, setAnalysis] = useState(null)
  const [lookupId, setLookupId] = useState('')
  const [teamForm, setTeamForm] = useState({ name: '', formation: '4-3-3' })
  const [addForm, setAddForm] = useState({ playerId: '', assignedSpot: 'CM', orderIndex: 0 })
  const [error, setError] = useState(null)
  const [busy, setBusy] = useState(false)

  const loadTeam = useCallback(async (id) => {
    if (!id) {
      setTeam(null)
      setRoster([])
      setAnalysis(null)
      return
    }
    setError(null)
    try {
      const [data, stats] = await Promise.all([fetchTeam(id), fetchTeamAnalysis(id)])
      setTeam(data.team)
      setRoster(data.players)
      setAnalysis(stats)
      setTeamForm({ name: data.team.name, formation: data.team.formation })
    } catch (e) {
      setError(e instanceof ApiError ? e.message : 'Failed to load team')
      setTeam(null)
    }
  }, [])

  useEffect(() => {
    if (selectedId) loadTeam(selectedId)
  }, [selectedId, loadTeam])

  const createNewTeam = async (e) => {
    e.preventDefault()
    if (!canWrite) return
    setBusy(true)
    setError(null)
    try {
      const data = await createTeam(teamForm)
      const id = data.team.id
      saveTeamId(id)
      const ids = loadSavedTeamIds()
      setTeamIds(ids)
      setSelectedId(id)
      await loadTeam(id)
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Create failed')
    } finally {
      setBusy(false)
    }
  }

  const saveTeam = async (e) => {
    e.preventDefault()
    if (!canWrite || !selectedId) return
    setBusy(true)
    setError(null)
    try {
      await updateTeam(selectedId, teamForm)
      await loadTeam(selectedId)
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Update failed')
    } finally {
      setBusy(false)
    }
  }

  const removeTeam = async () => {
    if (!canWrite || !selectedId || !confirm('Delete this team?')) return
    setBusy(true)
    setError(null)
    try {
      await deleteTeam(selectedId)
      removeSavedTeamId(selectedId)
      const ids = loadSavedTeamIds()
      setTeamIds(ids)
      setSelectedId(ids[0] ?? null)
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Delete failed')
    } finally {
      setBusy(false)
    }
  }

  const addPlayer = async (e) => {
    e.preventDefault()
    if (!selectedId) return
    setBusy(true)
    setError(null)
    try {
      await addTeamPlayer(selectedId, {
        playerId: Number(addForm.playerId),
        assignedSpot: addForm.assignedSpot,
        orderIndex: Number(addForm.orderIndex),
      })
      await loadTeam(selectedId)
      setAddForm((f) => ({ ...f, playerId: '' }))
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Could not add player')
    } finally {
      setBusy(false)
    }
  }

  const dropPlayer = async (playerId) => {
    if (!selectedId || !confirm('Remove player from team?')) return
    setBusy(true)
    setError(null)
    try {
      await removeTeamPlayer(selectedId, playerId)
      await loadTeam(selectedId)
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Remove failed')
    } finally {
      setBusy(false)
    }
  }

  const openById = () => {
    const id = Number(lookupId)
    if (!id) return
    saveTeamId(id)
    const ids = loadSavedTeamIds()
    setTeamIds(ids)
    setSelectedId(id)
    setLookupId('')
  }

  const isOwner = team && user && (team.ownerUserId === user.id || user.role === 'admin')

  return (
    <div className="page">
      <header className="page-header">
        <div>
          <h1>Teams &amp; pass simulation</h1>
          <p className="muted">
            Seeded teams <strong>#1</strong> (Riverside 4-3-3) and <strong>#2</strong> (Greenfield) are ready for pass
            simulation. Create more teams or load by ID; extra IDs are saved in this browser.
          </p>
        </div>
      </header>

      <Alert type="error" onDismiss={() => setError(null)}>
        {error}
      </Alert>

      <div className="filter-bar panel">
        <label>
          Open team by ID
          <input
            type="number"
            min={1}
            value={lookupId}
            onChange={(e) => setLookupId(e.target.value)}
            placeholder="e.g. 1"
          />
        </label>
        <button type="button" className="btn" onClick={openById}>
          Load
        </button>
      </div>

      <div className="split-layout">
        <section className="panel list-panel">
          <h2>Saved teams</h2>
          <ul className="entity-list">
            {teamIds.map((id) => (
              <li key={id}>
                <button
                  type="button"
                  className={`entity-row${selectedId === id ? ' active' : ''}`}
                  onClick={() => setSelectedId(id)}
                >
                  Team #{id}
                </button>
              </li>
            ))}
            {teamIds.length === 0 && <li className="empty">Create a team or load one by ID.</li>}
          </ul>

          {canWrite && (
            <form className="form-stack compact" onSubmit={createNewTeam}>
              <h3>Create team</h3>
              <label>
                Name
                <input
                  value={teamForm.name}
                  onChange={(e) => setTeamForm((f) => ({ ...f, name: e.target.value }))}
                  required
                />
              </label>
              <label>
                Formation
                <select
                  value={teamForm.formation}
                  onChange={(e) => setTeamForm((f) => ({ ...f, formation: e.target.value }))}
                  required
                >
                  {FORMATION_OPTIONS.map((f) => (
                    <option key={f.id} value={f.id}>
                      {f.label}
                    </option>
                  ))}
                </select>
              </label>
              <button type="submit" className="btn btn-primary" disabled={busy}>
                Create
              </button>
            </form>
          )}
        </section>

        <section className="panel detail-panel">
          {!team && <p className="empty">Select or load a team.</p>}

          {team && (
            <>
              <header className="inline-header">
                <h2>{team.name}</h2>
                <span className="muted">{team.formation}</span>
              </header>

              <div className="sim-howto panel">
                <h3>How to run the attack simulation</h3>
                <ol>
                  <li>Pick a formation and use <strong>Auto-pick best XI</strong>, then <strong>Apply lineup</strong>.</li>
                  <li>Swap any role via the dropdown if you want a different player.</li>
                  <li>Click <strong>Run attack</strong> — random risk per player, build-up toward opponent goal.</li>
                </ol>
              </div>

              {canWrite && isOwner && (
                <FormationLineup
                  teamId={selectedId}
                  formation={teamForm.formation}
                  canWrite={canWrite}
                  roster={roster}
                  onApplied={() => loadTeam(selectedId)}
                />
              )}

              <FormationSimulator teamId={selectedId} roster={roster} />

              {analysis && (
                <div className="analysis-cards">
                  <div className="analysis-card">
                    <span className="label">Players</span>
                    <strong>{analysis.playerCount}</strong>
                  </div>
                  <div className="analysis-card">
                    <span className="label">Avg composite</span>
                    <strong>{analysis.averageCompositeFiveStats?.toFixed?.(1) ?? analysis.averageCompositeFiveStats}</strong>
                  </div>
                  <div className="analysis-card">
                    <span className="label">Sum composite</span>
                    <strong>{analysis.sumCompositeFiveStats}</strong>
                  </div>
                </div>
              )}

              {canWrite && isOwner && (
                <form className="form-stack compact" onSubmit={saveTeam}>
                  <h3>Edit team</h3>
                  <label>
                    Name
                    <input
                      value={teamForm.name}
                      onChange={(e) => setTeamForm((f) => ({ ...f, name: e.target.value }))}
                      required
                    />
                  </label>
                  <label>
                    Formation
                    <select
                      value={teamForm.formation}
                      onChange={(e) => setTeamForm((f) => ({ ...f, formation: e.target.value }))}
                      required
                    >
                      {FORMATION_OPTIONS.map((f) => (
                        <option key={f.id} value={f.id}>
                          {f.label}
                        </option>
                      ))}
                    </select>
                  </label>
                  <div className="btn-row">
                    <button type="submit" className="btn btn-primary" disabled={busy}>
                      Update
                    </button>
                    <button type="button" className="btn btn-danger" onClick={removeTeam} disabled={busy}>
                      Delete team
                    </button>
                  </div>
                </form>
              )}

              <h3>Roster</h3>
              <ul className="roster-list">
                {roster.map((tp) => (
                  <li key={tp.id}>
                    <span>
                      Player #{tp.playerId} · {tp.assignedSpot} · order {tp.orderIndex}
                    </span>
                    {canWrite && isOwner && (
                      <button type="button" className="btn btn-ghost" onClick={() => dropPlayer(tp.playerId)}>
                        Remove
                      </button>
                    )}
                  </li>
                ))}
                {roster.length === 0 && <li className="empty">No players on this team.</li>}
              </ul>

              {canWrite && isOwner && (
                <form className="form-stack compact" onSubmit={addPlayer}>
                  <h3>Add player</h3>
                  <label>
                    Player ID
                    <input
                      type="number"
                      min={1}
                      value={addForm.playerId}
                      onChange={(e) => setAddForm((f) => ({ ...f, playerId: e.target.value }))}
                      required
                    />
                  </label>
                  <label>
                    Spot
                    <select
                      value={addForm.assignedSpot}
                      onChange={(e) => setAddForm((f) => ({ ...f, assignedSpot: e.target.value }))}
                    >
                      {PREFERRED_SPOTS.map((s) => (
                        <option key={s} value={s}>
                          {s}
                        </option>
                      ))}
                    </select>
                  </label>
                  <label>
                    Order
                    <input
                      type="number"
                      value={addForm.orderIndex}
                      onChange={(e) => setAddForm((f) => ({ ...f, orderIndex: e.target.value }))}
                    />
                  </label>
                  <button type="submit" className="btn btn-primary" disabled={busy}>
                    Add to team
                  </button>
                </form>
              )}
            </>
          )}
        </section>
      </div>
    </div>
  )
}
