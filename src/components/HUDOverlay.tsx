import React, { useEffect, useState } from 'react';
import { StyleSheet, Text, View, TouchableOpacity } from 'react-native';

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

  useEffect(() => {
    let intervalId: any;
    if (isActive) {
      intervalId = setInterval(() => {
        // Poll synchronous JSI metric binding if native module loaded
        if ((global as any).EmojiTrackerModule) {
          const data = (global as any).EmojiTrackerModule.getTelemetrySync();
          setTelemetry(data);
        }
      }, 16); // ~60 FPS HUD update
    }
    return () => clearInterval(intervalId);
  }, [isActive]);

  return (
    <View style={styles.hudContainer}>
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

      <TouchableOpacity
        style={[styles.button, isActive ? styles.stopBtn : styles.startBtn]}
        onPress={() => setIsActive(!isActive)}
      >
        <Text style={styles.buttonText}>{isActive ? 'PAUSE BOT' : 'START AUTOMATION'}</Text>
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
  button: {
    marginTop: 14,
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
