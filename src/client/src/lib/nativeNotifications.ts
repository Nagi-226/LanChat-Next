let permissionRequested = false;

export async function notifyNative(title: string, body: string): Promise<void> {
  if (!('Notification' in window)) return;

  let permission = Notification.permission;
  if (permission === 'default' && !permissionRequested) {
    permissionRequested = true;
    permission = await Notification.requestPermission();
  }
  if (permission !== 'granted') return;

  new Notification(title, {
    body,
    silent: false,
  });
}
