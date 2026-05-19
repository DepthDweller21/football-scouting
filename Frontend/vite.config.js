import { defineConfig } from 'vite'
import react from '@vitejs/plugin-react'

export default defineConfig({
  plugins: [react()],
  server: {
    proxy: {
      '/auth': 'http://127.0.0.1:8080',
      '/schools': 'http://127.0.0.1:8080',
      '/players': 'http://127.0.0.1:8080',
      '/visits': 'http://127.0.0.1:8080',
      '/reports': 'http://127.0.0.1:8080',
      '/teams': 'http://127.0.0.1:8080',
      '/formations': 'http://127.0.0.1:8080',
      '/health': 'http://127.0.0.1:8080',
      '/db-health': 'http://127.0.0.1:8080',
    },
  },
})
