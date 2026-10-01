'use strict';
const fs = require('node:fs');
const path = require('node:path');
const root = process.argv[2];
if (!root) throw new Error('Especifica el directorio de Xenia-WebServices');
function patch(file, before, after) {
  const target = path.join(root, file); const source = fs.readFileSync(target, 'utf8');
  if (source.includes(after)) return;
  if (!source.includes(before)) throw new Error('La versión upstream no coincide: ' + file);
  fs.writeFileSync(target, source.replace(before, after));
}
patch('src/main.ts', "await app.listen(PORT, '0.0.0.0');", "await app.listen(PORT, process.env.EMULOS_BIND_ADDRESS || '127.0.0.1');");
patch('src/infrastructure/presentation/AppLoggerMiddleware.ts', 'const headers_JSON = JSON.stringify(headers);', "const headers_JSON = '[private gateway]';");
patch('src/infrastructure/persistance/repositories/SessionRepository.ts', '}).limit(resultsCount);', '}); // MultiP: apply the limit after all matchmaking filters.');
patch('src/infrastructure/persistance/repositories/SessionRepository.ts', '    return sessions;\n', '    return sessions.slice(0, Math.max(1, Math.min(1000, resultsCount)));\n');
// Keep writes (QoS files) out of the installed binaries; each server has its own data directory.
patch('src/infrastructure/persistance/repositories/SessionRepository.ts', "        process.cwd(),\n        'qos',", "        process.env.EMULOS_STATE_DIR || process.cwd(),\n        'qos',");
for (const file of ['src/infrastructure/presentation/controllers/session.controller.ts', 'src/application/commandHandlers/DeleteSessionCommandHandler.ts']) {
  const target = path.join(root, file);
  if (fs.existsSync(target)) fs.writeFileSync(target, fs.readFileSync(target, 'utf8').replaceAll(/(?<!\|\| )process\.cwd\(\)/g, '(process.env.EMULOS_STATE_DIR || process.cwd())'));
}
