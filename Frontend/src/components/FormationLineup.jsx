import { useCallback, useEffect, useMemo, useState } from 'react'
import { ApiError } from '../api/client'
import {
  applyTeamLineup,
  fetchFormations,
  fetchSuggestLineup,
  fetchTopPlayers,
} from '../api/scouting'
import { formationTemplatesFallback } from '../constants'

function playerLabel(p) {
  if (!p) return '—'
  return `#${p.id} ${p.firstName} ${p.lastName} (${p.preferredSpot})`
}

function lineupHasDuplicatePlayers(rows) {
  const seen = new Set()
  for (const row of rows) {
    const id = Number(row.playerId)
    if (!id) continue
    if (seen.has(id)) return true
    seen.add(id)
  }
  return false
}

function mapRosterToSlots(templateSlots, roster) {
  const used = new Set()
  return templateSlots.map((slot) => {
    const exact = roster.find(
      (r) =>
        r.assignedSpot === slot.assignedSpot &&
        r.orderIndex === slot.orderIndex &&
        !used.has(r.playerId),
    )
    const fallback = roster.find((r) => r.assignedSpot === slot.assignedSpot && !used.has(r.playerId))
    const match = exact || fallback
    if (match?.playerId) used.add(match.playerId)
    return {
      orderIndex: slot.orderIndex,
      assignedSpot: slot.assignedSpot,
      playerId: match?.playerId ?? null,
    }
  })
}

export default function FormationLineup({ teamId, formation, canWrite, onApplied, schoolId, roster = [] }) {
  const [templates, setTemplates] = useState([])
  const [pool, setPool] = useState([])
  const [slots, setSlots] = useState([])
  const [error, setError] = useState(null)
  const [busy, setBusy] = useState(false)

  const template = useMemo(
    () => templates.find((t) => t.id === formation || t.formation === formation),
    [templates, formation],
  )

  const loadSuggestion = useCallback(async () => {
    if (!formation) return
    const data = await fetchSuggestLineup(formation, schoolId)
    const rows = (data.lineup || []).map((row) => ({
      orderIndex: row.orderIndex,
      assignedSpot: row.assignedSpot,
      playerId: Number(row.playerId) || null,
      fitScore: row.fitScore,
    }))
    if (lineupHasDuplicatePlayers(rows)) {
      throw new ApiError('Suggested lineup contained duplicate players')
    }
    setSlots(rows)
  }, [formation, schoolId])

  useEffect(() => {
    let cancelled = false
    ;(async () => {
      try {
        const players = await fetchTopPlayers({ limit: 50 })
        if (cancelled) return
        setPool(players)
      } catch (e) {
        if (!cancelled) {
          setError(e instanceof ApiError ? e.message : 'Failed to load players')
        }
      }
      try {
        const formData = await fetchFormations()
        if (!cancelled) setTemplates(formData.length > 0 ? formData : formationTemplatesFallback())
      } catch {
        if (!cancelled) setTemplates(formationTemplatesFallback())
      }
    })()
    return () => {
      cancelled = true
    }
  }, [])

  useEffect(() => {
    if (!template) return
    if (roster.length === template.slots.length) {
      const mapped = mapRosterToSlots(template.slots, roster)
      if (mapped.every((s) => s.playerId) && !lineupHasDuplicatePlayers(mapped)) {
        setSlots(mapped)
        return
      }
    }
    if (!formation || !canWrite) return
    let cancelled = false
    ;(async () => {
      setError(null)
      try {
        await loadSuggestion()
      } catch (e) {
        if (!cancelled) {
          setError(e instanceof ApiError ? e.message : 'Could not suggest lineup')
        }
      }
    })()
    return () => {
      cancelled = true
    }
  }, [formation, canWrite, loadSuggestion, template, roster])

  const usedIds = useMemo(() => new Set(slots.map((s) => s.playerId).filter(Boolean)), [slots])

  const optionsForSlot = (slotIndex) => {
    const currentId = slots[slotIndex]?.playerId
    return pool.filter((p) => p.id === currentId || !usedIds.has(p.id))
  }

  const setSlotPlayer = (index, playerId) => {
    const id = Number(playerId) || null
    setSlots((prev) =>
      prev.map((s, i) => {
        if (i === index) return { ...s, playerId: id }
        if (id && s.playerId === id) return { ...s, playerId: null }
        return s
      }),
    )
  }

  const applyLineup = async () => {
    if (!teamId || !formation || !canWrite) return
    const incomplete = slots.some((s) => !s.playerId)
    if (incomplete) {
      setError('Pick a player for every position before applying.')
      return
    }
    if (lineupHasDuplicatePlayers(slots)) {
      setError('Each player can only be selected once. Clear duplicates before applying.')
      return
    }
    setBusy(true)
    setError(null)
    try {
      const payload = {
        formation,
        lineup: slots.map((s) => ({
          playerId: s.playerId,
          assignedSpot: s.assignedSpot,
          orderIndex: s.orderIndex,
        })),
      }
      if (schoolId) payload.schoolId = schoolId
      await applyTeamLineup(teamId, payload)
      onApplied?.()
    } catch (e) {
      setError(e instanceof ApiError ? e.message : 'Could not apply lineup')
    } finally {
      setBusy(false)
    }
  }

  const autoPick = async () => {
    setBusy(true)
    setError(null)
    try {
      await loadSuggestion()
    } catch (e) {
      setError(e instanceof ApiError ? e.message : 'Could not auto-pick')
    } finally {
      setBusy(false)
    }
  }

  if (!formation) return null

  return (
    <section className="formation-lineup panel">
      <header className="inline-header">
        <h3>Lineup ({formation})</h3>
        {canWrite && (
          <button type="button" className="btn btn-ghost" onClick={autoPick} disabled={busy}>
            Auto-pick best XI
          </button>
        )}
      </header>
      <p className="muted small">
        Each role is filled with the best-matching player from the pool. Use the dropdown to swap someone in before
        applying.
      </p>

      {error && <p className="form-error">{error}</p>}

      {!template && templates.length > 0 && (
        <p className="form-error">Unknown formation — choose a preset below.</p>
      )}

      <ol className="lineup-slots">
        {slots.map((slot, index) => (
          <li key={`${slot.assignedSpot}-${slot.orderIndex}`}>
            <span className="lineup-spot">{slot.assignedSpot}</span>
            {canWrite ? (
              <select
                value={slot.playerId ?? ''}
                onChange={(e) => setSlotPlayer(index, e.target.value)}
                disabled={busy}
              >
                <option value="">Select player…</option>
                {optionsForSlot(index).map((p) => (
                  <option key={p.id} value={p.id}>
                    {playerLabel(p)}
                  </option>
                ))}
              </select>
            ) : (
              <span>{playerLabel(pool.find((p) => p.id === slot.playerId))}</span>
            )}
          </li>
        ))}
        {slots.length === 0 && <li className="empty">Loading lineup slots…</li>}
      </ol>

      {canWrite && slots.length > 0 && (
        <button type="button" className="btn btn-primary" onClick={applyLineup} disabled={busy}>
          Apply lineup to team
        </button>
      )}
    </section>
  )
}
