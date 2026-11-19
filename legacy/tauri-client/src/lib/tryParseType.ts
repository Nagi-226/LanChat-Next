export function tryParseType(json: string): number | null {
  try {
    const msg = JSON.parse(json);
    return typeof msg.type === 'number' ? msg.type : null;
  } catch {
    return null;
  }
}
