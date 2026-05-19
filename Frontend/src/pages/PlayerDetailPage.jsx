import { useEffect, useState } from 'react'
import { ApiError } from '../api/client'
import {
  createReport,
  createVisit,
  fetchPlayer,
  fetchPlayerReports,
  fetchSchools,
} from '../api/scouting'
import { useAuth } from '../context/AuthContext'
import Alert from '../components/Alert'
import { statTotal } from '../components/PlayerCard'

export default function PlayerDetailPage({ playerId, onBack }) {
  const { canWrite } = useAuth()
  const [player, setPlayer] = useState(null)
  const [schoolName, setSchoolName] = useState('')
  const [reports, setReports] = useState([])
  const [error, setError] = useState(null)
  const [busy, setBusy] = useState(false)

  const [visitForm, setVisitForm] = useState({ schoolId: '', visitDate: '', notes: '' })
  const [reportForm, setReportForm] = useState({
    recommendation: '',
    strengths: '',
    weaknesses: '',
  })

  useEffect(() => {
    if (!playerId) return
    setError(null)
    Promise.all([fetchPlayer(playerId), fetchPlayerReports(playerId), fetchSchools()])
      .then(([p, reps, schools]) => {
        setPlayer(p)
        setReports(reps)
        const school = schools.find((s) => s.id === p.schoolId)
        setSchoolName(school?.name || `School #${p.schoolId}`)
        setVisitForm((f) => ({ ...f, schoolId: String(p.schoolId), visitDate: new Date().toISOString().slice(0, 10) }))
      })
      .catch((e) => setError(e instanceof ApiError ? e.message : 'Failed to load player'))
  }, [playerId])

  const submitReport = async (e) => {
    e.preventDefault()
    if (!canWrite) return
    setBusy(true)
    setError(null)
    try {
      await createReport({
        playerId: Number(playerId),
        recommendation: reportForm.recommendation,
        strengths: reportForm.strengths,
        weaknesses: reportForm.weaknesses,
      })
      const reps = await fetchPlayerReports(playerId)
      setReports(reps)
      setReportForm({ recommendation: '', strengths: '', weaknesses: '' })
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Report failed')
    } finally {
      setBusy(false)
    }
  }

  const submitVisit = async (e) => {
    e.preventDefault()
    if (!canWrite) return
    setBusy(true)
    setError(null)
    try {
      await createVisit({
        schoolId: Number(visitForm.schoolId),
        visitDate: visitForm.visitDate,
        notes: visitForm.notes,
      })
      setVisitForm((f) => ({ ...f, notes: '' }))
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Visit failed')
    } finally {
      setBusy(false)
    }
  }

  if (!playerId) return null

  return (
    <div className="page">
      <button type="button" className="btn btn-ghost back-btn" onClick={onBack}>
        ← Back
      </button>

      <Alert type="error" onDismiss={() => setError(null)}>
        {error}
      </Alert>

      {!player && !error && <p className="empty">Loading…</p>}

      {player && (
        <>
          <header className="page-header">
            <div>
              <h1>
                {player.firstName} {player.lastName}
              </h1>
              <p className="muted">
                {schoolName} · {player.preferredSpot} · Age {player.age}
                {player.potentialScore >= 1 && player.potentialScore <= 100 && (
                  <> · Potential {player.potentialScore}</>
                )}
              </p>
            </div>
            <div className="stat-badge">Composite {statTotal(player)}</div>
          </header>

          <div className="stat-row large">
            <span>Shooting {player.shoot}</span>
            <span>Tackle {player.tackle}</span>
            <span>Speed {player.speed}</span>
            <span>Accuracy {player.accuracy}</span>
            <span>Awareness {player.awareness}</span>
          </div>

          <div className="split-layout">
            <section className="panel">
              <h2>Scouting reports</h2>
              <ul className="report-list">
                {reports.map((r) => (
                  <li key={r.id}>
                    <time>{r.createdAt}</time>
                    <p>
                      <strong>Recommendation:</strong> {r.recommendation || '—'}
                    </p>
                    <p>
                      <strong>Strengths:</strong> {r.strengths || '—'}
                    </p>
                    <p>
                      <strong>Weaknesses:</strong> {r.weaknesses || '—'}
                    </p>
                  </li>
                ))}
                {reports.length === 0 && <li className="empty">No reports yet.</li>}
              </ul>

              {canWrite && (
                <form className="form-stack compact" onSubmit={submitReport}>
                  <h3>Add report</h3>
                  <label>
                    Recommendation
                    <textarea
                      name="recommendation"
                      value={reportForm.recommendation}
                      onChange={(e) => setReportForm((f) => ({ ...f, recommendation: e.target.value }))}
                      rows={2}
                    />
                  </label>
                  <label>
                    Strengths
                    <textarea
                      name="strengths"
                      value={reportForm.strengths}
                      onChange={(e) => setReportForm((f) => ({ ...f, strengths: e.target.value }))}
                      rows={2}
                    />
                  </label>
                  <label>
                    Weaknesses
                    <textarea
                      name="weaknesses"
                      value={reportForm.weaknesses}
                      onChange={(e) => setReportForm((f) => ({ ...f, weaknesses: e.target.value }))}
                      rows={2}
                    />
                  </label>
                  <button type="submit" className="btn btn-primary" disabled={busy}>
                    Save report
                  </button>
                </form>
              )}
            </section>

            {canWrite && (
              <section className="panel">
                <h2>Log school visit</h2>
                <form className="form-stack compact" onSubmit={submitVisit}>
                  <label>
                    School ID
                    <input
                      name="schoolId"
                      type="number"
                      value={visitForm.schoolId}
                      onChange={(e) => setVisitForm((f) => ({ ...f, schoolId: e.target.value }))}
                      required
                    />
                  </label>
                  <label>
                    Date
                    <input
                      name="visitDate"
                      type="date"
                      value={visitForm.visitDate}
                      onChange={(e) => setVisitForm((f) => ({ ...f, visitDate: e.target.value }))}
                      required
                    />
                  </label>
                  <label>
                    Notes
                    <textarea
                      name="notes"
                      value={visitForm.notes}
                      onChange={(e) => setVisitForm((f) => ({ ...f, notes: e.target.value }))}
                      rows={3}
                    />
                  </label>
                  <button type="submit" className="btn btn-primary" disabled={busy}>
                    Record visit
                  </button>
                </form>
              </section>
            )}
          </div>
        </>
      )}
    </div>
  )
}
