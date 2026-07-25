import { TurboModule, TurboModuleRegistry } from 'react-native';

export interface TelemetryData {
  fps: number;
  processingTimeMs: number;
  predictedXLand: number;
  velocityX: number;
  velocityY: number;
  isTracking: boolean;
  anomalyDetected: boolean;
}

export interface Spec extends TurboModule {
  startScreenCapture(width: number, height: number): Promise<boolean>;
  stopScreenCapture(): void;
  setPaddleWidth(width: number): void;
  getTelemetrySync(): TelemetryData;
}

export default TurboModuleRegistry.getEnforcing<Spec>('EmojiTrackerModule');
