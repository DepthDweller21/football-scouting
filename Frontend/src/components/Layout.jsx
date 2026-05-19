import { useAuth } from '../context/AuthContext'

const NAV = [
  { id: 'schools', label: 'Schools' },
  { id: 'players', label: 'Players' },
  { id: 'teams', label: 'Teams & pass sim' },
]

export default function Layout({ page, onNavigate, children }) {
  const { user, logout, canWrite } = useAuth()

  return (
    <div className="app-shell">
      <header className="app-header">
        <button type="button" className="brand" onClick={() => onNavigate('schools')}>
          <span className="brand-mark" aria-hidden>
            ⚽
          </span>
          <span>
            <strong>Scout Desk</strong>
            <small>University scouting demo</small>
          </span>
        </button>
        <div className="header-meta">
          <span className={`role-badge role-${user.role}`}>{user.role}</span>
          <span className="user-email">{user.email}</span>
          <button type="button" className="btn btn-ghost" onClick={logout}>
            Log out
          </button>
        </div>
      </header>

      <div className="app-body">
        <nav className="sidebar" aria-label="Main">
          {NAV.map((item) => (
            <button
              key={item.id}
              type="button"
              className={`nav-link${page === item.id ? ' active' : ''}`}
              onClick={() => onNavigate(item.id)}
            >
              {item.label}
            </button>
          ))}
          {!canWrite && (
            <p className="sidebar-hint">Signed in as viewer — browse only. Register as scout to edit.</p>
          )}
        </nav>
        <main className="main-content">{children}</main>
      </div>
      </div>
  )
}
