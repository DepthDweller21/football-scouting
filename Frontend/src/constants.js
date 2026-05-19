export const PREFERRED_SPOTS = ['GK', 'RB', 'CB', 'LB', 'CDM', 'CM', 'CAM', 'RW', 'LW', 'ST']

/** Hard-coded formation presets (mirrors backend catalog). */
export const FORMATION_OPTIONS = [
  { id: '4-3-3', label: '4-3-3 (balanced attack)' },
  { id: '4-4-2', label: '4-4-2 (compact midfield)' },
  { id: '4-2-3-1', label: '4-2-3-1 (double pivot)' },
  { id: '3-5-2', label: '3-5-2 (wing play)' },
  { id: '5-3-2', label: '5-3-2 (defensive block)' },
  { id: '4-5-1', label: '4-5-1 (midfield control)' },
]

/** Slot order matches backend templates (left → right where on same rank). */
const FORMATION_SLOTS = {
  '4-3-3': ['GK', 'LB', 'CB', 'CB', 'RB', 'CM', 'CDM', 'CAM', 'LW', 'ST', 'RW'],
  '4-4-2': ['GK', 'LB', 'CB', 'CB', 'RB', 'LW', 'CM', 'CM', 'RW', 'ST', 'ST'],
  '4-2-3-1': ['GK', 'LB', 'CB', 'CB', 'RB', 'CDM', 'CDM', 'LW', 'CAM', 'RW', 'ST'],
  '3-5-2': ['GK', 'CB', 'CB', 'CB', 'CM', 'CDM', 'CM', 'LW', 'RW', 'ST', 'ST'],
  '5-3-2': ['GK', 'LB', 'CB', 'CB', 'CB', 'RB', 'CM', 'CM', 'CM', 'ST', 'ST'],
  '4-5-1': ['GK', 'LB', 'CB', 'CB', 'RB', 'CDM', 'CM', 'CAM', 'LW', 'RW', 'ST'],
}

/** Client-side formation templates when API is unavailable. */
export function formationTemplatesFallback() {
  return FORMATION_OPTIONS.map((f) => ({
    id: f.id,
    label: f.label,
    formation: f.id,
    slots: (FORMATION_SLOTS[f.id] || []).map((assignedSpot, orderIndex) => ({
      assignedSpot,
      orderIndex,
    })),
  }))
}

export const ROLES = [
  { value: 'viewer', label: 'Viewer (read only)' },
  { value: 'scout', label: 'Scout (read + write)' },
  { value: 'admin', label: 'Admin (full access)' },
]

export const TEAM_STORAGE_KEY = 'scouting_team_ids'

/** Seeded hypothetical team IDs (see Backend/data/teams.csv). */
export const SEED_TEAM_IDS = [1, 2]

