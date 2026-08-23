import React, { useEffect, useState, useRef } from "react";
import { StyleSheet, Text, TouchableOpacity, View, NativeModules, Alert, Animated } from "react-native";
import { useSafeAreaInsets } from "react-native-safe-area-context";
import NativeEmojiTracker from "../../modules/emoji-tracker/src";

export interface Telemetry {
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

export const HUDOverlay: React.FC = () => {
  const insets = useSafeAreaInsets();
  const [telemetry, setTelemetry] = useState<Telemetry>({
    fps: 0,
    processingTimeMs: 0,
    predictedXLand: 0,
    velocityX: 0,
    velocityY: 0,
    accelX: 0,
    accelY: 0,
    trackId: -1,
    trackCount: 0,
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
          trackerModule
            .getLatestTelemetry()
            .then((data: any) => {
              if (data) {
                setTelemetry({
                  fps: data.fps ?? 0,
                  processingTimeMs: data.processingTimeMs ?? 0,
                  predictedXLand: data.predictedXLand ?? 0,
                  velocityX: data.velocityX ?? 0,
                  velocityY: data.velocityY ?? 0,
                  accelX: data.accelX ?? 0,
                  accelY: data.accelY ?? 0,
                  trackId: data.trackId ?? -1,
                  trackCount: data.trackCount ?? 0,
                  isTracking: data.isTracking ?? false,
                  anomalyDetected: data.anomalyDetected ?? false,
                });
              }
            })
            .catch((err: any) => {
              console.warn("Failed to retrieve telemetry:", err);
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
          "Module Error",
          "EmojiTrackerModule native screen capture interface was not detected on this build. Please re-run prebuild.",
        );
        return;
      }
      try {
        await trackerModule.startScreenCapture(1080, 2400);
        setIsActive(true);
      } catch (err: any) {
        console.warn("Capture failed or denied:", err);
        Alert.alert("Permission Required", "Screen capture permission was not granted.");
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
      Alert.alert("Notice", "Overlay permission launcher not found.");
    }
  };

  const handleGrantAccessibility = () => {
    if (trackerModule?.requestAccessibilityPermission) {
      trackerModule.requestAccessibilityPermission();
    } else {
      Alert.alert("Notice", "Accessibility permission launcher not found.");
    }
  };

  // Smooth "live" pulse on the status dot while tracking
  const pulseAnim = useRef(new Animated.Value(0.35)).current;
  useEffect(() => {
    if (!telemetry.isTracking) {
      pulseAnim.setValue(0.35);
      return;
    }
    const loop = Animated.loop(
      Animated.sequence([
        Animated.timing(pulseAnim, { toValue: 1, duration: 700, useNativeDriver: true }),
        Animated.timing(pulseAnim, { toValue: 0.35, duration: 700, useNativeDriver: true }),
      ])
    );
    loop.start();
    return () => loop.stop();
  }, [telemetry.isTracking, pulseAnim]);

  // Derived display values
  const speed = Math.sqrt(telemetry.velocityX ** 2 + telemetry.velocityY ** 2);
  const accelMag = Math.sqrt(telemetry.accelX ** 2 + telemetry.accelY ** 2);
  const statusColor = !telemetry.isTracking ? "#64748b" : telemetry.anomalyDetected ? "#f59e0b" : "#34d399";
  const statusLabel = !telemetry.isTracking ? "IDLE" : telemetry.anomalyDetected ? "ANOMALY" : "LOCKED";

  return (
    <View style={[styles.hudContainer, { top: Math.max(insets.top + 10, 20) }]}>
      {/* Header */}
      <View style={styles.header}>
        <View style={styles.headerLeft}>
          <Animated.View style={[styles.statusDot, { backgroundColor: statusColor, opacity: pulseAnim }]} />
          <Text style={styles.title}>BOUNCE TRACER</Text>
        </View>
        <View style={[styles.statusPill, { borderColor: statusColor }]}>
          <Text style={[styles.statusText, { color: statusColor }]}>{statusLabel}</Text>
        </View>
      </View>

      {/* Primary metric: predicted landing */}
      <View style={styles.heroCard}>
        <Text style={styles.heroLabel}>TARGET X</Text>
        <Text style={styles.heroValue}>
          {telemetry.predictedXLand.toFixed(0)}
          <Text style={styles.heroUnit}> px</Text>
        </Text>
      </View>

      {/* Metric grid */}
      <View style={styles.grid}>
        <View style={styles.cell}>
          <Text style={styles.cellLabel}>FPS</Text>
          <Text style={styles.cellValue}>{telemetry.fps.toFixed(0)}</Text>
        </View>
        <View style={styles.cell}>
          <Text style={styles.cellLabel}>LATENCY</Text>
          <Text style={[styles.cellValue, telemetry.processingTimeMs > 16.7 && styles.cellWarn]}>
            {telemetry.processingTimeMs.toFixed(1)}
            <Text style={styles.cellUnit}>ms</Text>
          </Text>
        </View>
        <View style={styles.cell}>
          <Text style={styles.cellLabel}>SPEED</Text>
          <Text style={styles.cellValue}>{speed.toFixed(0)}</Text>
        </View>
        <View style={styles.cell}>
          <Text style={styles.cellLabel}>ACCEL</Text>
          <Text style={[styles.cellValue, accelMag > 3500 && styles.cellWarn]}>
            {accelMag.toFixed(0)}
          </Text>
        </View>
      </View>

      {/* Footer strip */}
      <View style={styles.footerStrip}>
        <Text style={styles.footerText}>
          TRACK #{telemetry.trackId} · {telemetry.trackCount} ACTIVE
        </Text>
        {telemetry.anomalyDetected && (
          <Text style={styles.anomalyText}>👽 NON-LINEAR</Text>
        )}
      </View>

      {/* Actions */}
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
        activeOpacity={0.8}
      >
        <Text style={styles.buttonText}>{isActive ? "■  STOP" : "▶  START"}</Text>
      </TouchableOpacity>
    </View>
  );
};

const styles = StyleSheet.create({
  hudContainer: {
    position: "absolute",
    top: 50,
    right: 20,
    width: 240,
    backgroundColor: "rgba(10, 14, 26, 0.88)",
    borderRadius: 20,
    padding: 16,
    borderWidth: 1,
    borderColor: "rgba(148, 163, 184, 0.15)",
    shadowColor: "#000",
    shadowOffset: { width: 0, height: 12 },
    shadowOpacity: 0.5,
    shadowRadius: 24,
    elevation: 12,
  },
  header: {
    flexDirection: "row",
    alignItems: "center",
    justifyContent: "space-between",
    marginBottom: 14,
  },
  headerLeft: {
    flexDirection: "row",
    alignItems: "center",
    gap: 8,
  },
  statusDot: {
    width: 8,
    height: 8,
    borderRadius: 4,
  },
  title: {
    color: "#e2e8f0",
    fontSize: 12,
    fontWeight: "700",
    letterSpacing: 1.5,
  },
  statusPill: {
    paddingHorizontal: 8,
    paddingVertical: 3,
    borderRadius: 999,
    borderWidth: 1,
    backgroundColor: "rgba(255,255,255,0.03)",
  },
  statusText: {
    fontSize: 9,
    fontWeight: "800",
    letterSpacing: 1,
  },
  heroCard: {
    backgroundColor: "rgba(56, 189, 248, 0.08)",
    borderRadius: 14,
    borderWidth: 1,
    borderColor: "rgba(56, 189, 248, 0.18)",
    paddingVertical: 12,
    paddingHorizontal: 14,
    alignItems: "center",
    marginBottom: 12,
  },
  heroLabel: {
    color: "#38bdf8",
    fontSize: 9,
    fontWeight: "700",
    letterSpacing: 2,
    marginBottom: 4,
  },
  heroValue: {
    color: "#f8fafc",
    fontSize: 32,
    fontWeight: "800",
    fontVariant: ["tabular-nums"],
  },
  heroUnit: {
    fontSize: 13,
    fontWeight: "600",
    color: "#64748b",
  },
  grid: {
    flexDirection: "row",
    flexWrap: "wrap",
    gap: 6,
    marginBottom: 10,
  },
  cell: {
    flexGrow: 1,
    flexBasis: "47%",
    backgroundColor: "rgba(255,255,255,0.04)",
    borderRadius: 10,
    paddingVertical: 8,
    paddingHorizontal: 10,
  },
  cellLabel: {
    color: "#64748b",
    fontSize: 8,
    fontWeight: "700",
    letterSpacing: 1.2,
    marginBottom: 2,
  },
  cellValue: {
    color: "#f1f5f9",
    fontSize: 15,
    fontWeight: "700",
    fontVariant: ["tabular-nums"],
  },
  cellUnit: {
    fontSize: 9,
    fontWeight: "600",
    color: "#64748b",
  },
  cellWarn: {
    color: "#fbbf24",
  },
  footerStrip: {
    flexDirection: "row",
    justifyContent: "space-between",
    alignItems: "center",
    paddingTop: 8,
    borderTopWidth: 1,
    borderTopColor: "rgba(148, 163, 184, 0.12)",
    marginBottom: 4,
  },
  footerText: {
    color: "#64748b",
    fontSize: 9,
    fontWeight: "600",
    letterSpacing: 0.8,
  },
  anomalyText: {
    color: "#f59e0b",
    fontSize: 9,
    fontWeight: "800",
    letterSpacing: 0.8,
  },
  permissionContainer: {
    marginTop: 10,
    gap: 6,
  },
  permBtn: {
    backgroundColor: "#3b82f6",
    paddingVertical: 7,
    borderRadius: 10,
    alignItems: "center",
  },
  permBtnText: {
    color: "#ffffff",
    fontSize: 11,
    fontWeight: "600",
  },
  permBtnSec: {
    backgroundColor: "rgba(255,255,255,0.05)",
    paddingVertical: 7,
    borderRadius: 10,
    alignItems: "center",
    borderWidth: 1,
    borderColor: "rgba(148, 163, 184, 0.18)",
  },
  permBtnTextSec: {
    color: "#94a3b8",
    fontSize: 11,
    fontWeight: "500",
  },
  button: {
    marginTop: 12,
    paddingVertical: 11,
    borderRadius: 12,
    alignItems: "center",
  },
  startBtn: {
    backgroundColor: "#0ea5e9",
    shadowColor: "#0ea5e9",
    shadowOffset: { width: 0, height: 4 },
    shadowOpacity: 0.35,
    shadowRadius: 10,
    elevation: 6,
  },
  stopBtn: {
    backgroundColor: "#ef4444",
    shadowColor: "#ef4444",
    shadowOffset: { width: 0, height: 4 },
    shadowOpacity: 0.35,
    shadowRadius: 10,
    elevation: 6,
  },
  buttonText: {
    color: "#ffffff",
    fontSize: 12,
    fontWeight: "800",
    letterSpacing: 1.2,
  },
});
