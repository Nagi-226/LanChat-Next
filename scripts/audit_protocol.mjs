import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const __dirname = path.dirname(fileURLToPath(import.meta.url));
const root = path.resolve(__dirname, '..');
const definitions = JSON.parse(fs.readFileSync(path.join(root, 'protocol', 'protocol_definitions.json'), 'utf8').replace(/^\uFEFF/, ''));
const schemaDir = path.join(root, 'protocol', 'schemas');
const ts = fs.readFileSync(path.join(root, 'protocol', 'message_types.ts'), 'utf8');
const header = fs.readFileSync(path.join(root, 'protocol', 'message_types.h'), 'utf8');

function fail(message) {
  throw new Error(message);
}

const schemaFiles = fs.readdirSync(schemaDir).filter((name) => name.endsWith('.schema.json')).sort();
if (schemaFiles.length !== definitions.messages.length) {
  fail(`Expected ${definitions.messages.length} schema files, found ${schemaFiles.length}.`);
}

const seenValues = new Set();
for (const message of definitions.messages) {
  if (seenValues.has(message.value)) {
    fail(`Duplicate message value ${message.value}.`);
  }
  seenValues.add(message.value);

  const schemaPath = path.join(schemaDir, `${message.schemaName}.schema.json`);
  if (!fs.existsSync(schemaPath)) {
    fail(`Missing schema for ${message.name}: ${schemaPath}`);
  }

  const schema = JSON.parse(fs.readFileSync(schemaPath, 'utf8'));
  const required = new Set(schema.required ?? []);
  const expectedRequired = ['type', ...message.required];

  if (schema.$schema !== definitions.schemaDraft) fail(`${message.schemaName}: unexpected schema draft.`);
  if (schema.additionalProperties !== false) fail(`${message.schemaName}: additionalProperties must be false.`);
  if (schema.properties?.type?.const !== message.value) fail(`${message.schemaName}: type const mismatch.`);
  for (const fieldName of expectedRequired) {
    if (!required.has(fieldName)) fail(`${message.schemaName}: missing required field ${fieldName}.`);
  }
  for (const fieldName of [...expectedRequired, ...definitions.commonFields, ...message.optional]) {
    if (!schema.properties?.[fieldName]) fail(`${message.schemaName}: missing property definition for ${fieldName}.`);
  }
}

for (const message of definitions.messages) {
  const enumPattern = new RegExp(`${message.name}\\s*=\\s*${message.value}`);
  if (!enumPattern.test(ts)) fail(`TypeScript enum missing ${message.name} = ${message.value}.`);
  if (!enumPattern.test(header)) fail(`C++ enum missing ${message.name} = ${message.value}.`);
}

const terminal = definitions.messages.at(-1);
if (terminal.name !== 'ProtocolHello' || terminal.value !== 50) {
  fail('Terminal protocol value must be ProtocolHello = 50 for v1.8.1+.');
}

console.log(`Protocol audit passed: ${definitions.messages.length} schemas, TypeScript enum, and C++ enum are aligned.`);


