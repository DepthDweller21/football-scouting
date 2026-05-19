const TOKEN_KEY = 'scouting_token'

export function getToken() {
  return localStorage.getItem(TOKEN_KEY)
}

export function setToken(token) {
  if (token) {
    localStorage.setItem(TOKEN_KEY, token)
  } else {
    localStorage.removeItem(TOKEN_KEY)
  }
}

/** Crow may return arrays as objects with numeric keys. */
export function toArray(value) {
  if (Array.isArray(value)) return value
  if (value && typeof value === 'object') return Object.values(value)
  return []
}

export class ApiError extends Error {
  constructor(message, { status, code } = {}) {
    super(message)
    this.name = 'ApiError'
    this.status = status
    this.code = code
  }
}

export async function api(path, { method = 'GET', body, auth = false } = {}) {
  const headers = { Accept: 'application/json' }
  if (body !== undefined) {
    headers['Content-Type'] = 'application/json'
  }
  if (auth) {
    const token = getToken()
    if (!token) {
      throw new ApiError('Not signed in', { status: 401, code: 'UNAUTHORIZED' })
    }
    headers.Authorization = `Bearer ${token}`
  }

  const res = await fetch(path, {
    method,
    headers,
    body: body !== undefined ? JSON.stringify(body) : undefined,
  })

  let json = null
  const text = await res.text()
  if (text) {
    try {
      json = JSON.parse(text)
    } catch {
      throw new ApiError('Invalid JSON from server', { status: res.status })
    }
  }

  if (!res.ok || (json && json.success === false)) {
    const message = json?.error?.message || json?.message || res.statusText || 'Request failed'
    const code = json?.error?.code
    throw new ApiError(message, { status: res.status, code })
  }

  return json?.data ?? json
}
