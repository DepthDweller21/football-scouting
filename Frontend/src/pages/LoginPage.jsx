import { useState } from 'react'
import { ApiError } from '../api/client'
import { useAuth } from '../context/AuthContext'
import { ROLES } from '../constants'
import Alert from '../components/Alert'

export default function LoginPage() {
  const { login, register } = useAuth()
  const [mode, setMode] = useState('login')
  const [busy, setBusy] = useState(false)
  const [message, setMessage] = useState(null)
  const [error, setError] = useState(null)

  const [form, setForm] = useState({
    fullName: '',
    email: '',
    password: '',
    role: 'scout',
  })

  const onChange = (e) => {
    setForm((f) => ({ ...f, [e.target.name]: e.target.value }))
  }

  const submit = async (e) => {
    e.preventDefault()
    setBusy(true)
    setError(null)
    setMessage(null)
    try {
      if (mode === 'login') {
        await login(form.email, form.password)
      } else {
        await register({
          fullName: form.fullName,
          email: form.email,
          password: form.password,
          role: form.role,
        })
        setMessage('Account created. Sign in with your email and password.')
        setMode('login')
      }
    } catch (err) {
      setError(err instanceof ApiError ? err.message : 'Something went wrong')
    } finally {
      setBusy(false)
    }
  }

  return (
    <div className="auth-page">
      <div className="auth-card">
        <div className="auth-hero">
          <span className="brand-mark large" aria-hidden>
            ⚽
          </span>
          <h1>Scout Desk</h1>
          <p>Track schools, rank players with AVL indexes, and build hypothetical squads.</p>
        </div>

        <div className="auth-panel">
          <div className="tabs">
            <button
              type="button"
              className={mode === 'login' ? 'active' : ''}
              onClick={() => setMode('login')}
            >
              Sign in
            </button>
            <button
              type="button"
              className={mode === 'register' ? 'active' : ''}
              onClick={() => setMode('register')}
            >
              Register
            </button>
          </div>

          <Alert type="success">{message}</Alert>
          <Alert type="error" onDismiss={() => setError(null)}>
            {error}
          </Alert>

          <form onSubmit={submit} className="form-stack">
            {mode === 'register' && (
              <>
                <label>
                  Full name
                  <input name="fullName" value={form.fullName} onChange={onChange} required />
                </label>
                <label>
                  Role
                  <select name="role" value={form.role} onChange={onChange}>
                    {ROLES.map((r) => (
                      <option key={r.value} value={r.value}>
                        {r.label}
                      </option>
                    ))}
                  </select>
                </label>
              </>
            )}
            <label>
              Email
              <input name="email" type="email" value={form.email} onChange={onChange} required />
            </label>
            <label>
              Password
              <input name="password" type="password" value={form.password} onChange={onChange} required />
            </label>
            <button type="submit" className="btn btn-primary" disabled={busy}>
              {busy ? 'Please wait…' : mode === 'login' ? 'Sign in' : 'Create account'}
            </button>
          </form>

          <p className="auth-footnote">
            Demo app — accounts are saved in Backend/data/users.csv (plain text). Run the C++ server on port 8080.
          </p>
        </div>
      </div>
    </div>
  )
}
