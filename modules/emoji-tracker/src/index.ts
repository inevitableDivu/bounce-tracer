import { NativeModules } from 'react-native';

const NativeEmojiTracker = NativeModules.EmojiTrackerModule || (globalThis as any).EmojiTrackerModule;

export default NativeEmojiTracker;
