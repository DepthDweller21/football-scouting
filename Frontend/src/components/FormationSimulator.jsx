import { useCallback, useEffect, useMemo, useRef, useState } from 'react'
import { ApiError, toArray } from '../api/client'
import { fetchTeamSimulation } from '../api/scouting'

const PASS_MS = 700

function normalizeSim(data) {
  const nodes = toArray(data?.nodes).map((n) => ({
    ...n,
    playerId: Number(n.playerId),
    xPct: Number(n.xPct),
    yPct: Number(n.yPct),
    riskFactor: Number(n.riskFactor ?? 50),
  }))
  const passes = toArray(data?.passes)
    .map((p) => ({
      from: Number(p.from ?? p.fromPlayerId),
      to: Number(p.to ?? p.toPlayerId),
      reasoning: p.reasoning ?? '',
      candidates: toArray(p.candidates).map((c) => ({
        playerId: Number(c.playerId),
        name: c.name,
        riskFactor: Number(c.riskFactor),
        effectiveRisk: Number(c.effectiveRisk ?? c.riskFactor),
        totalScore: Number(c.totalScore),
        safetyFromRisk: Number(c.safetyFromRisk),
        compositePart: Number(c.compositePart),
        forwardBonus: Number(c.forwardBonus),
        goalProximity: Number(c.goalProximity),
        selected: Boolean(c.selected),
      })),
      riskSnapshot: toArray(p.riskSnapshot).map((r) => ({
        playerId: Number(r.playerId),
        riskFactor: Number(r.riskFactor),
      })),
    }))
    .filter((p) => Number.isFinite(p.from) && Number.isFinite(p.to))

  const goal = data?.opponentGoal
    ? { xPct: Number(data.opponentGoal.xPct ?? 50), yPct: Number(data.opponentGoal.yPct ?? 6) }
    : { xPct: 50, yPct: 6 }

  return {
    ...data,
    nodes,
    edges: toArray(data?.edges),
    passes,
    opponentGoal: goal,
    scored: Boolean(data?.scored),
    outcome: data?.outcome ?? '',
    riskSeed: data?.riskSeed,
  }
}

function resolveStartNodeId(nodes, startPlayerId) {
  if (startPlayerId) {
    const id = Number(startPlayerId)
    if (nodes.some((n) => Number(n.playerId) === id)) return id
  }
  const gk = nodes.find((n) => n.rank === 0)
  return gk ? Number(gk.playerId) : Number(nodes[0]?.playerId)
}

function riskClass(risk) {
  if (risk <= 33) return 'risk-low'
  if (risk <= 66) return 'risk-mid'
  return 'risk-high'
}

function applyRiskSnapshot(nodes, snapshot) {
  if (!snapshot?.length) return nodes
  const byId = new Map(snapshot.map((r) => [Number(r.playerId), Number(r.riskFactor)]))
  return nodes.map((n) => ({
    ...n,
    riskFactor: byId.has(n.playerId) ? byId.get(n.playerId) : n.riskFactor,
  }))
}

export default function FormationSimulator({ teamId, roster }) {
  const [sim, setSim] = useState(null)
  const [startPlayerId, setStartPlayerId] = useState('')
  const [maxSteps, setMaxSteps] = useState(12)
  const [error, setError] = useState(null)
  const [busy, setBusy] = useState(false)
  const [playing, setPlaying] = useState(false)
  const [passIndex, setPassIndex] = useState(-1)
  const [expandedPass, setExpandedPass] = useState(null)
  const [ball, setBall] = useState({ x: 50, y: 88 })
  const [activePassLine, setActivePassLine] = useState(null)
  const cancelAnimRef = useRef(false)
  const rafRef = useRef(null)

  const nodeById = useCallback(
    (id) => sim?.nodes?.find((n) => Number(n.playerId) === Number(id)),
    [sim],
  )

  const cancelAnimation = useCallback(() => {
    cancelAnimRef.current = true
    if (rafRef.current != null) {
      cancelAnimationFrame(rafRef.current)
      rafRef.current = null
    }
  }, [])

  const placeBallOnNode = useCallback(
    (id) => {
      const n = nodeById(id)
      if (n) {
        setBall({ x: Number(n.xPct), y: Number(n.yPct) })
      }
    },
    [nodeById],
  )

  const animatePass = useCallback(
    (fromNode, toNode) =>
      new Promise((resolve) => {
        const fx = Number(fromNode.xPct)
        const fy = Number(fromNode.yPct)
        const tx = Number(toNode.xPct)
        const ty = Number(toNode.yPct)
        const t0 = performance.now()

        const tick = (now) => {
          if (cancelAnimRef.current) {
            resolve()
            return
          }
          const raw = Math.min(1, (now - t0) / PASS_MS)
          const t = raw * (2 - raw)
          setBall({
            x: fx + (tx - fx) * t,
            y: fy + (ty - fy) * t,
          })
          if (raw < 1) {
            rafRef.current = requestAnimationFrame(tick)
          } else {
            setBall({ x: tx, y: ty })
            rafRef.current = null
            resolve()
          }
        }
        rafRef.current = requestAnimationFrame(tick)
      }),
    [],
  )

  const runSimulation = async () => {
    if (!teamId) return
    cancelAnimation()
    setPlaying(false)
    setPassIndex(-1)
    setExpandedPass(null)
    setActivePassLine(null)
    setBusy(true)
    setError(null)
    try {
      const raw = await fetchTeamSimulation(teamId, {
        startPlayerId: startPlayerId ? Number(startPlayerId) : undefined,
        maxSteps: Number(maxSteps) || 12,
      })
      const data = normalizeSim(raw)
      setSim(data)
      const startId = resolveStartNodeId(data.nodes, startPlayerId)
      placeBallOnNode(startId)
    } catch (e) {
      setError(e instanceof ApiError ? e.message : 'Simulation failed')
      setSim(null)
    } finally {
      setBusy(false)
    }
  }

  const playPasses = useCallback(async () => {
    if (!sim?.passes?.length || !sim?.nodes?.length) return

    cancelAnimation()
    cancelAnimRef.current = false
    setPlaying(true)
    setPassIndex(-1)
    setExpandedPass(null)
    setActivePassLine(null)

    const startId = resolveStartNodeId(sim.nodes, startPlayerId)
    placeBallOnNode(startId)

    await new Promise((r) => setTimeout(r, 50))

    for (let i = 0; i < sim.passes.length; i++) {
      if (cancelAnimRef.current) break

      const pass = sim.passes[i]
      const from = nodeById(pass.from)
      const to = nodeById(pass.to)
      if (!from || !to) continue

      setPassIndex(i)
      setExpandedPass(i)
      setActivePassLine({ from: pass.from, to: pass.to })

      setBall({ x: Number(from.xPct), y: Number(from.yPct) })
      await new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)))
      await animatePass(from, to)

      if (cancelAnimRef.current) break
      await new Promise((r) => setTimeout(r, 80))
    }

    setActivePassLine(null)
    setPlaying(false)
  }, [sim, startPlayerId, nodeById, placeBallOnNode, animatePass, cancelAnimation])

  useEffect(() => () => cancelAnimation(), [cancelAnimation])

  useEffect(() => {
    if (!teamId) {
      setSim(null)
      return
    }
    if (roster?.length >= 2) {
      runSimulation()
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [teamId, roster?.length])

  const activeEdge =
    activePassLine != null ? `${activePassLine.from}-${activePassLine.to}` : null

  const displayNodes = useMemo(() => {
    if (!sim?.nodes?.length) return []
    const idx = passIndex >= 0 ? passIndex : 0
    const snap = sim.passes?.[idx]?.riskSnapshot
    const base = snap?.length ? applyRiskSnapshot(sim.nodes, snap) : sim.nodes

    // Separate any nodes that still share the same grid point (legacy rosters).
    const buckets = new Map()
    for (const n of base) {
      const key = `${n.xPct.toFixed(2)}:${n.yPct.toFixed(2)}`
      if (!buckets.has(key)) buckets.set(key, [])
      buckets.get(key).push(n)
    }
    return base.map((n) => {
      const key = `${n.xPct.toFixed(2)}:${n.yPct.toFixed(2)}`
      const group = buckets.get(key) || [n]
      if (group.length <= 1) return n
      const i = group.indexOf(n)
      const spread = Math.min(4, (group.length - 1) * 2.5)
      const offset = (i - (group.length - 1) / 2) * spread
      return { ...n, xPct: n.xPct + offset }
    })
  }, [sim, passIndex])

  const goal = sim?.opponentGoal ?? { xPct: 50, yPct: 6 }

  return (
    <section className="formation-sim panel">
      <header className="inline-header">
        <h3>Attack simulation</h3>
        <span className="muted">Risk picks pass targets; lower risk = safer outlet (not success %)</span>
      </header>

      {error && <p className="form-error">{error}</p>}

      <div className="sim-controls">
        <label>
          Start player ID
          <input
            type="number"
            min={1}
            placeholder="GK default"
            value={startPlayerId}
            onChange={(e) => setStartPlayerId(e.target.value)}
          />
        </label>
        <label>
          Max steps
          <input
            type="number"
            min={1}
            max={32}
            value={maxSteps}
            onChange={(e) => setMaxSteps(e.target.value)}
          />
        </label>
        <button type="button" className="btn" onClick={runSimulation} disabled={busy || !teamId}>
          {busy ? 'Running…' : 'Run attack'}
        </button>
        <button
          type="button"
          className="btn btn-primary"
          onClick={playPasses}
          disabled={!sim?.passes?.length || playing}
        >
          {playing ? 'Playing…' : 'Animate attack'}
        </button>
      </div>

      {!sim && !busy && roster?.length < 2 && (
        <p className="empty">Add at least two players to simulate an attack.</p>
      )}

      {sim && (
        <>
          {sim.outcome && (
            <p className={`attack-outcome${sim.scored ? ' scored' : ''}`}>
              {sim.scored ? '⚽ ' : ''}
              {sim.outcome}
              {sim.riskSeed != null && (
                <span className="muted"> · risk seed {sim.riskSeed}</span>
              )}
            </p>
          )}

          <p className="muted small pitch-meta">
            {displayNodes.length} player{displayNodes.length === 1 ? '' : 's'} on pitch
            {displayNodes.length > 0 && displayNodes.length < 11
              ? ' — apply a full lineup for 11'
              : ''}
          </p>

          <div className="pitch-wrap">
            <svg className="pitch-svg" viewBox="0 0 100 105" preserveAspectRatio="xMidYMid meet">
              <rect x="2" y="2" width="96" height="101" rx="2" className="pitch-border" />
              <line x1="2" y1="52.5" x2="98" y2="52.5" className="pitch-line" />
              <circle cx="50" cy="52.5" r="8" className="pitch-line" fill="none" />

              <g transform={`translate(${goal.xPct}, ${goal.yPct})`}>
                <rect x="-6" y="-2.5" width="12" height="5" className="opponent-goal" />
                <text y="-4" textAnchor="middle" className="goal-label">
                  GOAL
                </text>
              </g>

              {(() => {
                const seen = new Set()
                return sim.edges
                  ?.filter((e) => {
                    const a = Math.min(e.from, e.to)
                    const b = Math.max(e.from, e.to)
                    const key = `${a}-${b}`
                    if (seen.has(key)) return false
                    seen.add(key)
                    return true
                  })
                  .map((e) => {
                    const from = nodeById(e.from)
                    const to = nodeById(e.to)
                    if (!from || !to) return null
                    const key = `${e.from}-${e.to}`
                    const hot =
                      activeEdge === `${e.from}-${e.to}` || activeEdge === `${e.to}-${e.from}`
                    return (
                      <line
                        key={key}
                        x1={from.xPct}
                        y1={from.yPct}
                        x2={to.xPct}
                        y2={to.yPct}
                        className={hot ? 'pitch-edge hot' : 'pitch-edge'}
                      />
                    )
                  })
              })()}

              {activePassLine && (() => {
                const from = nodeById(activePassLine.from)
                const to = nodeById(activePassLine.to)
                if (!from || !to) return null
                return (
                  <line
                    x1={from.xPct}
                    y1={from.yPct}
                    x2={to.xPct}
                    y2={to.yPct}
                    className="pitch-pass-line"
                  />
                )
              })()}

              {displayNodes.map((n) => (
                <g key={n.playerId} transform={`translate(${n.xPct}, ${n.yPct})`}>
                  <circle r="3.2" className={`pitch-node ${riskClass(n.riskFactor)}`} />
                  <text y="-4.5" textAnchor="middle" className="pitch-label">
                    {n.spot}
                  </text>
                  <text y="6.5" textAnchor="middle" className={`pitch-risk ${riskClass(n.riskFactor)}`}>
                    R{n.riskFactor}
                  </text>
                </g>
              ))}

              <g transform={`translate(${ball.x}, ${ball.y})`}>
                <circle r="1.8" className="pitch-ball" />
              </g>
            </svg>
          </div>

          <ul className="pass-log">
            {sim.passes?.length === 0 && <li className="empty">No passes — attack stalled immediately.</li>}
            {sim.passes?.map((p, i) => {
              const from = nodeById(p.from)
              const to = nodeById(p.to)
              const open = expandedPass === i
              return (
                <li key={`${p.from}-${p.to}-${i}`} className={i === passIndex ? 'active' : ''}>
                  <button
                    type="button"
                    className="pass-log-header"
                    onClick={() => setExpandedPass(open ? null : i)}
                  >
                    <span>
                      {i + 1}. {from?.name ?? p.from} → {to?.name ?? p.to}
                    </span>
                    <span className="pass-log-toggle">{open ? '−' : '+'}</span>
                  </button>
                  {open && (
                    <div className="pass-log-detail">
                      <p className="pass-reasoning">{p.reasoning}</p>
                      <table className="candidate-table">
                        <thead>
                          <tr>
                            <th>Player</th>
                            <th>Risk</th>
                            <th>Eff.</th>
                            <th>Safety</th>
                            <th>Skill</th>
                            <th>Fwd</th>
                            <th>Goal Δ</th>
                            <th>Total</th>
                          </tr>
                        </thead>
                        <tbody>
                          {p.candidates?.map((c) => (
                            <tr key={c.playerId} className={c.selected ? 'selected-row' : ''}>
                              <td>{c.name}{c.selected ? ' ✓' : ''}</td>
                              <td>{c.riskFactor}</td>
                              <td>{c.effectiveRisk}</td>
                              <td>{c.safetyFromRisk}</td>
                              <td>{c.compositePart}</td>
                              <td>{c.forwardBonus}</td>
                              <td>{c.goalProximity}</td>
                              <td>
                                <strong>{c.totalScore}</strong>
                              </td>
                            </tr>
                          ))}
                        </tbody>
                      </table>
                    </div>
                  )}
                </li>
              )
            })}
          </ul>

          <p className="muted sim-legend">
            Scoring favors forward play: large forward bonus, heavy backward/GK-to-keeper penalties, front-line
            targets get −25 effective risk, home GK +25. Risk shifts ±20 after each pass.
          </p>
        </>
      )}
    </section>
  )
}
