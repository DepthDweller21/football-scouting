import { api, toArray } from './client'

export async function registerUser(payload) {
  return api('/auth/register', { method: 'POST', body: payload })
}

export async function loginUser(email, password) {
  return api('/auth/login', { method: 'POST', body: { email, password } })
}

export async function logoutUser() {
  return api('/auth/logout', { method: 'POST', auth: true })
}

export async function fetchMe() {
  return api('/auth/me', { auth: true })
}

export async function fetchSchools() {
  const data = await api('/schools')
  return toArray(data?.schools)
}

export async function fetchSchool(id) {
  const data = await api(`/schools/${id}`)
  return data?.school
}

export async function fetchSchoolPlayers(schoolId) {
  const data = await api(`/schools/${schoolId}/players`)
  return toArray(data?.players)
}

export async function createSchool(payload) {
  return api('/schools', { method: 'POST', body: payload, auth: true })
}

export async function updateSchool(id, payload) {
  return api(`/schools/${id}`, { method: 'PUT', body: payload, auth: true })
}

export async function deleteSchool(id) {
  return api(`/schools/${id}`, { method: 'DELETE', auth: true })
}

function queryString(params) {
  const q = new URLSearchParams()
  Object.entries(params).forEach(([k, v]) => {
    if (v !== undefined && v !== null && v !== '') q.set(k, String(v))
  })
  const s = q.toString()
  return s ? `?${s}` : ''
}

export async function fetchTopPlayers(filters = {}) {
  const data = await api(`/players/top${queryString(filters)}`)
  return toArray(data?.players)
}

export async function fetchPotentialPlayers(filters = {}) {
  const data = await api(`/players/potential${queryString(filters)}`)
  return toArray(data?.players)
}

export async function fetchPlayer(id) {
  const data = await api(`/players/${id}`)
  return data?.player
}

export async function createPlayer(payload) {
  return api('/players', { method: 'POST', body: payload, auth: true })
}

export async function createVisit(payload) {
  return api('/visits', { method: 'POST', body: payload, auth: true })
}

export async function fetchPlayerReports(playerId) {
  const data = await api(`/reports/player/${playerId}`)
  return toArray(data?.reports)
}

export async function createReport(payload) {
  return api('/reports', { method: 'POST', body: payload, auth: true })
}

export async function createTeam(payload) {
  return api('/teams', { method: 'POST', body: payload, auth: true })
}

export async function fetchTeam(id) {
  const data = await api(`/teams/${id}`)
  return {
    team: data?.team,
    players: toArray(data?.players),
  }
}

export async function updateTeam(id, payload) {
  return api(`/teams/${id}`, { method: 'PUT', body: payload, auth: true })
}

export async function deleteTeam(id) {
  return api(`/teams/${id}`, { method: 'DELETE', auth: true })
}

export async function addTeamPlayer(teamId, payload) {
  return api(`/teams/${teamId}/players`, { method: 'POST', body: payload, auth: true })
}

export async function removeTeamPlayer(teamId, playerId) {
  return api(`/teams/${teamId}/players/${playerId}`, { method: 'DELETE', auth: true })
}

export async function fetchTeamAnalysis(teamId) {
  return api(`/teams/${teamId}/analysis`)
}

export async function fetchTeamSimulation(teamId, params = {}) {
  return api(`/teams/${teamId}/simulate${queryString(params)}`)
}

export async function fetchFormations() {
  const data = await api('/formations')
  return toArray(data?.formations)
}

export async function fetchSuggestLineup(formationId, schoolId) {
  const params = schoolId ? { schoolId } : {}
  return api(`/formations/${encodeURIComponent(formationId)}/suggest-lineup${queryString(params)}`)
}

export async function applyTeamLineup(teamId, payload) {
  return api(`/teams/${teamId}/lineup`, { method: 'POST', body: payload, auth: true })
}
