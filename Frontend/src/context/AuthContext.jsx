import { createContext, useCallback, useContext, useEffect, useMemo, useState } from 'react'
import { ApiError, getToken, setToken } from '../api/client'
import { fetchMe, loginUser, logoutUser, registerUser } from '../api/scouting'

const AuthContext = createContext(null)

export function AuthProvider({ children }) {
  const [user, setUser] = useState(null)
  const [loading, setLoading] = useState(true)
  const [error, setError] = useState(null)

  const refresh = useCallback(async () => {
    if (!getToken()) {
      setUser(null)
      setLoading(false)
      return
    }
    try {
      const data = await fetchMe()
      setUser(data?.user ?? null)
      setError(null)
    } catch (e) {
      setUser(null)
      setToken(null)
      if (e instanceof ApiError && e.status === 401) {
        setError(null)
      } else {
        setError(e.message)
      }
    } finally {
      setLoading(false)
    }
  }, [])

  useEffect(() => {
    refresh()
  }, [refresh])

  const login = async (email, password) => {
    setError(null)
    const data = await loginUser(email, password)
    setToken(data.token)
    setUser(data.user)
    return data.user
  }

  const register = async (payload) => {
    setError(null)
    const data = await registerUser(payload)
    return data.user
  }

  const logout = async () => {
    try {
      await logoutUser()
    } catch {
      /* clear local session even if server call fails */
    }
    setToken(null)
    setUser(null)
  }

  const canWrite = user?.role === 'admin' || user?.role === 'scout'

  const value = useMemo(
    () => ({
      user,
      loading,
      error,
      canWrite,
      login,
      register,
      logout,
      refresh,
      setError,
    }),
    [user, loading, error, canWrite, refresh],
  )

  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>
}

export function useAuth() {
  const ctx = useContext(AuthContext)
  if (!ctx) throw new Error('useAuth must be used within AuthProvider')
  return ctx
}
