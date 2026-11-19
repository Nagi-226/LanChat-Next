// barrel export — 4 independent stores
import { useStore } from 'zustand';
import type { AuthState, CurrentUser, FriendRequestInfo, SearchResult } from './types';

export { useAuthStore } from './authStore';
export { useMessageStore } from './messageStore';
export { useFriendStore } from './friendStore';
export { useSearchStore } from './searchStore';
export type { CurrentUser, AuthState, FriendRequestInfo, SearchResult } from './types';
