export function statTotal(p) {
  return (p.shoot || 0) + (p.tackle || 0) + (p.speed || 0) + (p.accuracy || 0) + (p.awareness || 0)
}

export default function PlayerCard({ player, onSelect }) {
  const total = statTotal(player)
  return (
    <article className="player-card">
      <button type="button" className="player-card-btn" onClick={() => onSelect?.(player.id)}>
        <div className="player-card-head">
          <h3>
            {player.firstName} {player.lastName}
          </h3>
          <span className="spot-pill">{player.preferredSpot}</span>
        </div>
        <p className="muted">
          School #{player.schoolId} · Age {player.age}
          {player.potentialScore >= 1 && player.potentialScore <= 100 && (
            <> · Potential {player.potentialScore}</>
          )}
        </p>
        <div className="stat-row">
          <span>SH {player.shoot}</span>
          <span>TK {player.tackle}</span>
          <span>SP {player.speed}</span>
          <span>AC {player.accuracy}</span>
          <span>AW {player.awareness}</span>
          <strong className="stat-total">Σ {total}</strong>
        </div>
      </button>
    </article>
  )
}
