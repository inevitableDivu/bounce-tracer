import React, { useEffect, useState } from 'react';
import { StyleSheet, Text, TouchableOpacity, View, NativeModules, Alert } from 'react-native';
import { useSafeAreaInsets } from 'react-native-safe-area-context';
import NativeEmojiTracker from '../../modules/emoji-tracker/src';

export interface Telemetry {
  fps: number;
  processingTimeMs: number;
  predictedXLand: number;
  velocityX: number;
  velocityY: number;
  isTracking: boolean;
  anomalyDetected: boolean;
}

export const HUDOverlay: React.FC = () => {
  const insets = useSafeAreaInsets();
  const [telemetry, setTelemetry] = useState<Telemetry>({
    fps: 0,
    processingTimeMs: 0,
    predictedXLand: 0,
    velocityX: 0,
    velocityY: 0,
    isTracking: false,
    anomalyDetected: false,
  });
  const [isActive, setIsActive] = useState<boolean>(false);
  const [hasOverlayPerm, setHasOverlayPerm] = useState<boolean>(true);

  const trackerModule = NativeEmojiTracker || NativeModules.EmojiTrackerModule;

  useEffect(() => {
    if (trackerModule?.isOverlayPermissionGranted) {
      trackerModule.isOverlayPermissionGranted().then((granted: boolean) => {
        setHasOverlayPerm(granted);
      });
    }
  }, [trackerModule]);

  useEffect(() => {
    let intervalId: ReturnType<typeof setInterval>;
    if (isActive) {
      intervalId = setInterval(() => {
        if (trackerModule?.getLatestTelemetry) {
          trackerModule.getLatestTelemetry()
            .then((data: any) => {
              if (data) {
                setTelemetry({
                  fps: data.fps ?? 0,
                  processingTimeMs: data.processingTimeMs ?? 0,
                  predictedXLand: data.predictedXLand ?? 0,
                  velocityX: data.velocityX ?? 0,
                  velocityY: data.velocityY ?? 0,
                  isTracking: data.isTracking ?? false,
                  anomalyDetected: data.anomalyDetected ?? false,
                });
              }
            })
            .catch((err: any) => {
              console.warn('Failed to retrieve telemetry:', err);
            });
        }
      }, 100);
    }
    return () => clearInterval(intervalId);
  }, [isActive, trackerModule]);

  const handleToggleAutomation = async () => {
    if (!isActive) {
      if (!trackerModule?.startScreenCapture) {
        Alert.alert(
          'Module Error',
          'EmojiTrackerModule native screen capture interface was not detected on this build. Please re-run prebuild.'
        );
        return;
      }
      try {
        await trackerModule.startScreenCapture(1080, 2400);
        setIsActive(true);
      } catch (err: any) {
        console.warn('Capture failed or denied:', err);
        Alert.alert('Permission Required', 'Screen capture permission was not granted.');
      }
    } else {
      if (trackerModule?.stopScreenCapture) {
        trackerModule.stopScreenCapture();
      }
      setIsActive(false);
    }
  };

  const handleGrantOverlay = () => {
    if (trackerModule?.requestOverlayPermission) {
      trackerModule.requestOverlayPermission();
    } else {
      Alert.alert('Notice', 'Overlay permission launcher not found.');
    }
  };

  const handleGrantAccessibility = () => {
    if (trackerModule?.requestAccessibilityPermission) {
      trackerModule.requestAccessibilityPermission();
    } else {
      Alert.alert('Notice', 'Accessibility permission launcher not found.');
    }
  };

  return (
    <View style={[styles.hudContainer, { top: Math.max(insets.top + 10, 20) }]}>
      <Text style={styles.title}>🤖 BOUNCE TRACER BOT</Text>
      
      <View style={styles.row}>
        <Text style={styles.label}>FPS:</Text>
        <Text style={styles.value}>{telemetry.fps.toFixed(1)}</Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Latency:</Text>
        <Text style={styles.value}>{telemetry.processingTimeMs.toFixed(2)} ms</Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Target X:</Text>
        <Text style={styles.value}>{telemetry.predictedXLand.toFixed(1)} px</Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Velocity (Vx, Vy):</Text>
        <Text style={styles.value}>
          ({telemetry.velocityX.toFixed(0)}, {telemetry.velocityY.toFixed(0)})
        </Text>
      </View>
      <View style={styles.row}>
        <Text style={styles.label}>Status:</Text>
        <Text style={[styles.value, telemetry.isTracking ? styles.tracking : styles.idle]}>
          {telemetry.isTracking ? 'TRACKING' : 'IDLE'}
        </Text>
      </View>

      {telemetry.anomalyDetected && (
        <View style={styles.anomalyBadge}>
          <Text style={styles.anomalyText}>⚠️ ANOMALY DETECTED (👽)</Text>
        </View>
      )}

      <View style={styles.permissionContainer}>
        {!hasOverlayPerm && (
          <TouchableOpacity style={styles.permBtn} onPress={handleGrantOverlay}>
            <Text style={styles.permBtnText}>Enable Display Over Apps</Text>
          </TouchableOpacity>
        )}
        <TouchableOpacity style={styles.permBtnSec} onPress={handleGrantAccessibility}>
          <Text style={styles.permBtnTextSec}>Enable Touch Accessibility</Text>
        </TouchableOpacity>
      </View>

      <TouchableOpacity
        style={[styles.button, isActive ? styles.stopBtn : styles.startBtn]}
        onPress={handleToggleAutomation}
      >
        <Text style={styles.buttonText}>{isActive ? 'PAUSE AUTOMATION' : 'START AUTOMATION'}</Text>
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  hudContainer: {
    position: 'absolute',
    top: 50,
    right: 20,
    width: 260,
    backgroundColor: 'rgba(15, 23, 42, 0.92)',
    borderRadius: 16,
    padding: 16,
    borderWidth: 1,
    borderColor: 'rgba(56, 189, 248, 0.4)',
    shadowColor: '#000',
    shadowOffset: { width: 0, height: 8 },
    shadowOpacity: 0.4,
    shadowRadius: 12,
    elevation: 10,
  },
  title: {
    color: '#38bdf8',
    fontSize: 14,
    fontWeight: 'bold',
    marginBottom: 12,
    letterSpacing: 0.5,
  },
  row: {
    flexDirection: 'row',
    justifyContent: 'space-between',
    marginBottom: 6,
  },
  label: {
    color: '#94a3b8',
    fontSize: 12,
  },
  value: {
    color: '#f8fafc',
    fontSize: 12,
    fontWeight: '600',
  },
  tracking: {
    color: '#4ade80',
  },
  idle: {
    color: '#f87171',
  },
  anomalyBadge: {
    backgroundColor: 'rgba(239, 68, 68, 0.2)',
    borderColor: '#ef4444',
    borderWidth: 1,
    borderRadius: 6,
    paddingVertical: 4,
    paddingHorizontal: 8,
    marginTop: 8,
  },
  anomalyText: {
    color: '#fca5a5',
    fontSize: 10,
    fontWeight: 'bold',
    textAlign: 'center',
  },
  permissionContainer: {
    marginTop: 10,
    gap: 6,
  },
  permBtn: {
    backgroundColor: '#3b82f6',
    paddingVertical: 6,
    borderRadius: 6,
    alignItems: 'center',
  },
  permBtnText: {
    color: '#ffffff',
    fontSize: 11,
    fontWeight: '600',
  },
  permBtnSec: {
    backgroundColor: '#334155',
    paddingVertical: 6,
    borderRadius: 6,
    alignItems: 'center',
  },
  permBtnTextSec: {
    color: '#94a3b8',
    fontSize: 11,
    fontWeight: '500',
  },
  button: {
    marginTop: 12,
    paddingVertical: 10,
    borderRadius: 8,
    alignItems: 'center',
  },
  startBtn: {
    backgroundColor: '#0284c7',
  },
  stopBtn: {
    backgroundColor: '#dc2626',
  },
  buttonText: {
    color: '#ffffff',
    fontSize: 12,
    fontWeight: 'bold',
    letterSpacing: 0.5,
  },
});
