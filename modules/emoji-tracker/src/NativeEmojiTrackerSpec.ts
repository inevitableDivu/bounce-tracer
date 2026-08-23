import { TurboModule, TurboModuleRegistry } from 'react-native';

export interface TelemetryData {
  fps: number;
  processingTimeMs: number;
  predictedXLand: number;
  velocityX: number;
  velocityY: number;
  accelX: number;
  accelY: number;
  trackId: number;
  trackCount: number;
  isTracking: boolean;
  anomalyDetected: boolean;
}

export interface CalibrationData {
  screenWidth: number;
  screenHeight: number;
  paddleY: number;
  frameRateHz: number;
  leadFrames: number;
  restitution: number;
}

export interface Spec extends TurboModule {
  startScreenCapture(width: number, height: number): Promise<boolean>;
  stopScreenCapture(): void;
  setPaddleWidth(width: number): void;
  setCalibration(cal: CalibrationData): void;
  getLatestTelemetry(): Promise<TelemetryData>;
  isOverlayPermissionGranted(): Promise<boolean>;
  requestOverlayPermission(): void;
  requestAccessibilityPermission(): void;
}

export default TurboModuleRegistry.getEnforcing<Spec>('EmojiTrackerModule');
