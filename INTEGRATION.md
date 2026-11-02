# Web integration (C++ REST API + React frontend)

## Run (Windows, VS Code terminal)
1. Backend (from this folder):
   server\build_server.bat
   bin\HospitalServer.exe
   -> http://127.0.0.1:8080/api   (options: bin\HospitalServer.exe --help)
2. Frontend (second terminal):
   cd frontend
   npm install
   npm run dev
   -> http://localhost:8443   (backend URL is VITE_API_BASE_URL in frontend/.env.development)

## Tests
- Original C++ suite: build_and_test.bat (41 tests)
- API integration tests (31): server\build_server.bat, then  node --test server/tests/api.test.mjs
- Frontend: cd frontend && npx tsc --noEmit && npm run build

State is in memory; `POST /api/admin/reset` (or restarting the server) restores the seed data.
