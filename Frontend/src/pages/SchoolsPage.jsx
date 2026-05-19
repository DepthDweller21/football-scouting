import { useCallback, useEffect, useState } from 'react'
import { ApiError } from '../api/client'
import {
  createSchool,
  deleteSchool,
  fetchSchool,
  fetchSchoolPlayers,
  fetchSchools,
  updateSchool,
} from '../api/scouting'
import { useAuth } from '../context/AuthContext'
import Alert from '../components/Alert'
import PlayerCard from '../components/PlayerCard'

const emptySchoolForm = { name: '', location: '', contactEmail: '' }

export default function SchoolsPage({ onOpenPlayer }) {
  const { canWrite } = useAuth()
  const [schools, setSchools] = useState([])
  const [selectedId, setSelectedId] = useState(null)
  const [players, setPlayers] = useState([])
  const [form, setForm] = useState(emptySchoolForm)
  const [editing, setEditing] = useState(false)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)
  const [busy, setBusy] = useState(false)

  const loadSchools = useCallback(async () => {
    setLoading(true)
    setError(null)
    try {
      const list = await fetchSchools()
      setSchools(list)
      setSelectedId((prev) => prev ?? list[0]?.id ?? null)
    } catch (e) {
      setError(e instanceof ApiError ? e.message : 'Failed to load schools')
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    loadSchools()
  }, [loadSchools])

  useEffect(() => {
    if (!selectedId) {
      setPlayers([])
      return
    }
    fetchSchoolPlayers(selectedId)
      .then(setPlayers)
      .catch(() => setPlayers([]))
  }, [selectedId])

  const selectSchool = async (id) => {
    setSelectedId(id)
    setEditing(false)
    try {
      const s = await fetchSchool(id)
      setForm({
        name: s.name,
        location: s.location,
        contactEmail: s.contactEmail || '',
      })
    } catch {
      setForm(emptySchoolForm)
    }
  }

  const onFormChange = (e) => {
    setForm((f) => ({ ...f, [e.target.name]: e.target.value }))
  }

  const saveSchool = async (e) => {
    e.preventDefault()
    if (!canWrite) return
    setBusy(true)
    setError(null)
    try {
      const payload = {
        name: form.name,
        location: form.location,
        contactEmail: form.contactEmail || undefined,
      }
      if (editing && selectedId) {
        await updateSchool(selectedId, payload)
      } else {
        const res = await createSchool(payload)
        setSelectedId(res.id)
        setEditing(true)
      }
      await loadSchools()
      if (selectedId) await selectSchool(selectedId)
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Save failed')
    } finally {
      setBusy(false)
    }
  }

  const removeSchool = async () => {
    if (!canWrite || !selectedId || !confirm('Delete this school?')) return
    setBusy(true)
    setError(null)
    try {
      await deleteSchool(selectedId)
      setSelectedId(null)
      setForm(emptySchoolForm)
      setEditing(false)
      await loadSchools()
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Delete failed')
    } finally {
      setBusy(false)
    }
  }

  const startCreate = () => {
    setSelectedId(null)
    setEditing(false)
    setForm(emptySchoolForm)
  }

  return (
    <div className="page">
      <header className="page-header">
        <div>
          <h1>Schools</h1>
          <p className="muted">Browse academies and their registered players.</p>
        </div>
        {canWrite && (
          <button type="button" className="btn btn-primary" onClick={startCreate}>
            New school
          </button>
        )}
      </header>

      <Alert type="error" onDismiss={() => setError(null)}>
        {error}
      </Alert>

      <div className="split-layout">
        <section className="panel list-panel">
          <h2>All schools {loading && <span className="muted">…</span>}</h2>
          <ul className="entity-list">
            {schools.map((s) => (
              <li key={s.id}>
                <button
                  type="button"
                  className={`entity-row${selectedId === s.id ? ' active' : ''}`}
                  onClick={() => selectSchool(s.id)}
                >
                  <strong>{s.name}</strong>
                  <span className="muted">{s.location}</span>
                </button>
              </li>
            ))}
            {!loading && schools.length === 0 && (
              <li className="empty">No schools yet. Create one to get started.</li>
            )}
          </ul>
        </section>

        <section className="panel detail-panel">
          {canWrite && (
            <form className="form-stack compact" onSubmit={saveSchool}>
              <h2>{editing ? 'Edit school' : 'Add school'}</h2>
              <label>
                Name
                <input name="name" value={form.name} onChange={onFormChange} required />
              </label>
              <label>
                Location
                <input name="location" value={form.location} onChange={onFormChange} required />
              </label>
              <label>
                Contact email
                <input name="contactEmail" type="email" value={form.contactEmail} onChange={onFormChange} />
              </label>
              <div className="btn-row">
                <button type="submit" className="btn btn-primary" disabled={busy}>
                  {editing ? 'Update' : 'Create'}
                </button>
                {editing && selectedId && (
                  <button type="button" className="btn btn-danger" onClick={removeSchool} disabled={busy}>
                    Delete
                  </button>
                )}
              </div>
            </form>
          )}

          {selectedId && (
            <div className="subsection">
              <h2>Players at this school</h2>
              <div className="card-grid">
                {players.map((p) => (
                  <PlayerCard key={p.id} player={p} onSelect={onOpenPlayer} />
                ))}
                {players.length === 0 && <p className="empty">No players linked to this school.</p>}
              </div>
            </div>
          )}
        </section>
      </div>
    </div>
  )
}
