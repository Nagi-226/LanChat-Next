// LanChat-Next protocol message types. Keep aligned with protocol/message_types.h.
export enum MsgType {
  RegisterUser = 0,
  RegisterUserReturn = 1,
  Login = 2,
  LoginSuccessReturn = 3,
  LoginFailedReturn = 4,
  SendMsg = 5,
  ReceiveMsg = 6,
  UserOnline = 7,
  UserOffline = 8,
  CreateGroup = 9,
  CreateGroupReturn = 10,
  SearchGroup = 11,
  SearchGroupReturn = 12,
  JoinGroup = 13,
  JoinGroupReturn = 14,
  LeaveGroup = 15,
  SendGroupMsg = 16,
  ReceiveGroupMsg = 17,
  UserJoinGroup = 18,
  UserLeaveGroup = 19,
  Heartbeat = 20,
  HeartbeatAck = 21,
  OfflineMessages = 22,
  Logout = 23,
  RequestHistory = 24,
  HistoryResponse = 25,
  SendFile = 26,
  ReceiveFile = 27,
  FileTransferDone = 28,
  AIRequest = 29,
  AIResponse = 30,
  AIStreamChunk = 31,
  UserProfileUpdate = 32,
  SystemBroadcast = 33,
}

export type ContentType = 'text' | 'image' | 'file' | 'system';
export type AIRequestType = 'summarize' | 'translate' | 'search' | 'chat';

export interface UserInfo {
  id: number;
  nickname: string;
  headId: number;
}

export interface BaseMessage {
  type: MsgType;
  timestamp?: number;
  msg_id?: string;
}

export interface AuthMessage extends BaseMessage {
  type: MsgType.RegisterUser | MsgType.Login;
  id?: number;
  password: string;
  nickname?: string;
  headId?: number;
}

export interface ChatMessage extends BaseMessage {
  type: MsgType.SendMsg | MsgType.ReceiveMsg | MsgType.SendGroupMsg | MsgType.ReceiveGroupMsg;
  fromId?: number;
  toId?: number;
  groupId?: number;
  id?: number;
  headId?: number;
  nickname?: string;
  msg: string;
  content_type?: ContentType;
}

export interface AIMessage extends BaseMessage {
  type: MsgType.AIRequest | MsgType.AIResponse | MsgType.AIStreamChunk;
  request_id: string;
  ai_type?: AIRequestType;
  status?: 'start' | 'done' | 'error';
  chunk_index?: number;
  is_final?: boolean;
  msg?: string;
}

export type ProtocolMessage = BaseMessage | AuthMessage | ChatMessage | AIMessage;

export const MSG_TYPE_NAMES: Record<MsgType, string> = {
  [MsgType.RegisterUser]: 'registerUser',
  [MsgType.RegisterUserReturn]: 'registerUserReturn',
  [MsgType.Login]: 'login',
  [MsgType.LoginSuccessReturn]: 'loginSucReturn',
  [MsgType.LoginFailedReturn]: 'loginLoseReturn',
  [MsgType.SendMsg]: 'sendMsg',
  [MsgType.ReceiveMsg]: 'receiveMsg',
  [MsgType.UserOnline]: 'userOnline',
  [MsgType.UserOffline]: 'userOffline',
  [MsgType.CreateGroup]: 'createGroup',
  [MsgType.CreateGroupReturn]: 'createGroupReturn',
  [MsgType.SearchGroup]: 'searchGroup',
  [MsgType.SearchGroupReturn]: 'searchGroupReturn',
  [MsgType.JoinGroup]: 'joinGroup',
  [MsgType.JoinGroupReturn]: 'joinGroupReturn',
  [MsgType.LeaveGroup]: 'leaveGroup',
  [MsgType.SendGroupMsg]: 'sendGroupMsg',
  [MsgType.ReceiveGroupMsg]: 'receiveGroupMsg',
  [MsgType.UserJoinGroup]: 'userJoinGroup',
  [MsgType.UserLeaveGroup]: 'userLeaveGroup',
  [MsgType.Heartbeat]: 'heartbeat',
  [MsgType.HeartbeatAck]: 'heartbeatAck',
  [MsgType.OfflineMessages]: 'offlineMessages',
  [MsgType.Logout]: 'logout',
  [MsgType.RequestHistory]: 'requestHistory',
  [MsgType.HistoryResponse]: 'historyResponse',
  [MsgType.SendFile]: 'sendFile',
  [MsgType.ReceiveFile]: 'receiveFile',
  [MsgType.FileTransferDone]: 'fileTransferDone',
  [MsgType.AIRequest]: 'aiRequest',
  [MsgType.AIResponse]: 'aiResponse',
  [MsgType.AIStreamChunk]: 'aiStreamChunk',
  [MsgType.UserProfileUpdate]: 'userProfileUpdate',
  [MsgType.SystemBroadcast]: 'systemBroadcast',
};

export function isValidMsgType(value: number): value is MsgType {
  return Number.isInteger(value) && value >= MsgType.RegisterUser && value <= MsgType.SystemBroadcast;
}
