import { useState } from 'react'
import { AuthProvider, useAuth } from './context/AuthContext'
import Layout from './components/Layout'
import LoginPage from './pages/LoginPage'
import SchoolsPage from './pages/SchoolsPage'
import PlayersPage from './pages/PlayersPage'
import PlayerDetailPage from './pages/PlayerDetailPage'
import TeamsPage from './pages/TeamsPage'
import './App.css'

function AppRoutes() {
  const { user, loading } = useAuth()
  const [page, setPage] = useState('schools')
  const [playerId, setPlayerId] = useState(null)

  if (loading) {
    return (
      <div className="loading-screen">
        <p>Loading…</p>
      </div>
    )
  }

  if (!user) {
    return <LoginPage />
  }

  const openPlayer = (id) => {
    setPlayerId(id)
    setPage('player-detail')
  }

  const goBackFromPlayer = () => {
    setPlayerId(null)
    setPage('players')
  }

  let content
  if (page === 'player-detail') {
    content = <PlayerDetailPage playerId={playerId} onBack={goBackFromPlayer} />
  } else if (page === 'schools') {
    content = <SchoolsPage onOpenPlayer={openPlayer} />
  } else if (page === 'players') {
    content = <PlayersPage onOpenPlayer={openPlayer} />
  } else if (page === 'teams') {
    content = <TeamsPage />
  } else {
    content = <SchoolsPage onOpenPlayer={openPlayer} />
  }

  return (
    <Layout
      page={page === 'player-detail' ? 'players' : page}
      onNavigate={(p) => {
        setPlayerId(null)
        setPage(p)
      }}
    >
      {content}
    </Layout>
  )
}

export default function App() {
  return (
    <AuthProvider>
      <AppRoutes />
    </AuthProvider>
  )
}
