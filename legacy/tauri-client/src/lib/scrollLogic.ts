export interface ScrollMetrics {
  scrollTop: number;
  scrollHeight: number;
  clientHeight: number;
}

export function isNearScrollBottom(metrics: ScrollMetrics, thresholdPx = 80): boolean {
  const distanceFromBottom = metrics.scrollHeight - metrics.scrollTop - metrics.clientHeight;
  return distanceFromBottom <= thresholdPx;
}

export function nextUnreadCount(
  currentUnread: number,
  previousMessageCount: number,
  nextMessageCount: number,
  nearBottom: boolean,
): number {
  if (nearBottom) return 0;
  return currentUnread + Math.max(0, nextMessageCount - previousMessageCount);
}
