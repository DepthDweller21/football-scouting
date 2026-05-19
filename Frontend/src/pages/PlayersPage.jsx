import { useCallback, useEffect, useState } from 'react'
import { ApiError } from '../api/client'
import {
  createPlayer,
  fetchPotentialPlayers,
  fetchSchools,
  fetchTopPlayers,
} from '../api/scouting'
import { useAuth } from '../context/AuthContext'
import { PREFERRED_SPOTS } from '../constants'
import Alert from '../components/Alert'
import PlayerCard from '../components/PlayerCard'

const defaultFilters = { limit: 20, schoolId: '', preferredSpot: '' }

const defaultPlayerForm = {
  schoolId: '',
  firstName: '',
  lastName: '',
  age: 18,
  preferredSpot: 'CM',
  potentialScore: '',
  shoot: 50,
  tackle: 50,
  speed: 50,
  accuracy: 50,
  awareness: 50,
}

export default function PlayersPage({ onOpenPlayer }) {
  const { canWrite } = useAuth()
  const [tab, setTab] = useState('top')
  const [schools, setSchools] = useState([])
  const [players, setPlayers] = useState([])
  const [filters, setFilters] = useState(defaultFilters)
  const [form, setForm] = useState(defaultPlayerForm)
  const [showForm, setShowForm] = useState(false)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)
  const [busy, setBusy] = useState(false)

  useEffect(() => {
    fetchSchools().then(setSchools).catch(() => setSchools([]))
  }, [])

  const loadPlayers = useCallback(async () => {
    setLoading(true)
    setError(null)
    const q = { limit: filters.limit }
    if (filters.schoolId) q.schoolId = filters.schoolId
    if (filters.preferredSpot) q.preferredSpot = filters.preferredSpot
    try {
      const list =
        tab === 'top' ? await fetchTopPlayers(q) : await fetchPotentialPlayers(q)
      setPlayers(list)
    } catch (e) {
      setError(e instanceof ApiError ? e.message : 'Failed to load players')
    } finally {
      setLoading(false)
    }
  }, [tab, filters])

  useEffect(() => {
    loadPlayers()
  }, [loadPlayers])

  const onFilterChange = (e) => {
    const { name, value } = e.target
    setFilters((f) => ({ ...f, [name]: value }))
  }

  const onFormChange = (e) => {
    const { name, value } = e.target
    setForm((f) => ({ ...f, [name]: value }))
  }

  const submitPlayer = async (e) => {
    e.preventDefault()
    if (!canWrite) return
    setBusy(true)
    setError(null)
    try {
      const payload = {
        schoolId: Number(form.schoolId),
        firstName: form.firstName,
        lastName: form.lastName,
        age: Number(form.age),
        preferredSpot: form.preferredSpot,
        shoot: Number(form.shoot),
        tackle: Number(form.tackle),
        speed: Number(form.speed),
        accuracy: Number(form.accuracy),
        awareness: Number(form.awareness),
      }
      if (form.potentialScore !== '') {
        payload.potentialScore = Number(form.potentialScore)
      }
      const res = await createPlayer(payload)
      setShowForm(false)
      setForm(defaultPlayerForm)
      await loadPlayers()
      if (res?.id) onOpenPlayer(res.id)
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Could not create player')
    } finally {
      setBusy(false)
    }
  }

  return (
    <div className="page">
      <header className="page-header">
        <div>
          <h1>Players</h1>
          <p className="muted">Rankings from AVL trees — composite stats and potential.</p>
        </div>
        {canWrite && (
          <button type="button" className="btn btn-primary" onClick={() => setShowForm((v) => !v)}>
            {showForm ? 'Close form' : 'Add player'}
          </button>
        )}
      </header>

      <Alert type="error" onDismiss={() => setError(null)}>
        {error}
      </Alert>

      <div className="tabs inline-tabs">
        <button type="button" className={tab === 'top' ? 'active' : ''} onClick={() => setTab('top')}>
          Top composite
        </button>
        <button
          type="button"
          className={tab === 'potential' ? 'active' : ''}
          onClick={() => setTab('potential')}
        >
          Top potential
        </button>
      </div>

      <div className="filter-bar panel">
        <label>
          Limit
          <input name="limit" type="number" min={1} max={100} value={filters.limit} onChange={onFilterChange} />
        </label>
        <label>
          School
          <select name="schoolId" value={filters.schoolId} onChange={onFilterChange}>
            <option value="">All schools</option>
            {schools.map((s) => (
              <option key={s.id} value={s.id}>
                {s.name}
              </option>
            ))}
          </select>
        </label>
        <label>
          Position
          <select name="preferredSpot" value={filters.preferredSpot} onChange={onFilterChange}>
            <option value="">Any</option>
            {PREFERRED_SPOTS.map((s) => (
              <option key={s} value={s}>
                {s}
              </option>
            ))}
          </select>
        </label>
        <button type="button" className="btn" onClick={loadPlayers}>
          Refresh
        </button>
      </div>

      {showForm && canWrite && (
        <form className="panel form-stack" onSubmit={submitPlayer}>
          <h2>New player</h2>
          <div className="form-grid">
            <label>
              School
              <select name="schoolId" value={form.schoolId} onChange={onFormChange} required>
                <option value="">Select…</option>
                {schools.map((s) => (
                  <option key={s.id} value={s.id}>
                    {s.name}
                  </option>
                ))}
              </select>
            </label>
            <label>
              First name
              <input name="firstName" value={form.firstName} onChange={onFormChange} required />
            </label>
            <label>
              Last name
              <input name="lastName" value={form.lastName} onChange={onFormChange} required />
            </label>
            <label>
              Age
              <input name="age" type="number" min={10} max={40} value={form.age} onChange={onFormChange} required />
            </label>
            <label>
              Position
              <select name="preferredSpot" value={form.preferredSpot} onChange={onFormChange}>
                {PREFERRED_SPOTS.map((s) => (
                  <option key={s} value={s}>
                    {s}
                  </option>
                ))}
              </select>
            </label>
            <label>
              Potential (1–100, optional)
              <input name="potentialScore" type="number" min={1} max={100} value={form.potentialScore} onChange={onFormChange} />
            </label>
            {['shoot', 'tackle', 'speed', 'accuracy', 'awareness'].map((stat) => (
              <label key={stat}>
                {stat}
                <input name={stat} type="number" min={1} max={100} value={form[stat]} onChange={onFormChange} required />
              </label>
            ))}
          </div>
          <button type="submit" className="btn btn-primary" disabled={busy}>
            Create player
          </button>
        </form>
      )}

      <section className="card-grid">
        {loading && <p className="empty">Loading rankings…</p>}
        {!loading &&
          players.map((p) => <PlayerCard key={p.id} player={p} onSelect={onOpenPlayer} />)}
        {!loading && players.length === 0 && <p className="empty">No players match these filters.</p>}
      </section>
    </div>
  )
}
