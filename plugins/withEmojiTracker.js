const {
  withAndroidManifest,
  withDangerousMod,
  createRunOncePlugin,
} = require('@expo/config-plugins');
const fs = require('fs');
const path = require('path');

const PKG_NAME = 'expo-instagram-emoji-bot';
const PKG_VERSION = '1.0.0';

/**
 * Ensures required permissions and Accessibility Service entry exist in AndroidManifest.xml
 */
const withCustomManifest = (config) => {
  return withAndroidManifest(config, async (config) => {
    const androidManifest = config.modResults;
    const mainApplication = androidManifest.manifest.application[0];

    if (!androidManifest.manifest['uses-permission']) {
      androidManifest.manifest['uses-permission'] = [];
    }

    const permissions = [
      'android.permission.SYSTEM_ALERT_WINDOW',
      'android.permission.FOREGROUND_SERVICE',
      'android.permission.FOREGROUND_SERVICE_MEDIA_PROJECTION',
      'android.permission.BIND_ACCESSIBILITY_SERVICE',
    ];

    permissions.forEach((perm) => {
      if (
        !androidManifest.manifest['uses-permission'].some(
          (p) => p.$['android:name'] === perm
        )
      ) {
        androidManifest.manifest['uses-permission'].push({
          $: { 'android:name': perm },
        });
      }
    });

    if (!mainApplication.service) {
      mainApplication.service = [];
    }

    const serviceName = 'com.inevitabledivu.bouncetracer.InstagramEmojiAccessibilityService';
    const hasService = mainApplication.service.some(
      (s) => s.$['android:name'] === serviceName
    );

    if (!hasService) {
      mainApplication.service.push({
        $: {
          'android:name': serviceName,
          'android:permission': 'android.permission.BIND_ACCESSIBILITY_SERVICE',
          'android:exported': 'true',
        },
        'intent-filter': [
          {
            action: [
              {
                $: {
                  'android:name':
                    'android.accessibilityservice.AccessibilityService',
                },
              },
            ],
          },
        ],
        'meta-data': [
          {
            $: {
              'android:name': 'android.accessibilityservice',
              'android:resource': '@xml/accessibility_service_config',
            },
          },
        ],
      });
    }

    const captureServiceName = 'com.inevitabledivu.bouncetracer.ScreenCaptureService';
    const hasCaptureService = mainApplication.service.some(
      (s) => s.$['android:name'] === captureServiceName
    );

    if (!hasCaptureService) {
      mainApplication.service.push({
        $: {
          'android:name': captureServiceName,
          'android:exported': 'false',
          'android:foregroundServiceType': 'mediaProjection',
        },
      });
    }

    return config;
  });
};

/**
 * Copies the accessibility configuration XML to res/xml during prebuild
 */
const withAccessibilityConfigXml = (config) => {
  return withDangerousMod(config, [
    'android',
    async (config) => {
      const resXmlDir = path.join(
        config.modRequest.platformProjectRoot,
        'app/src/main/res/xml'
      );

      if (!fs.existsSync(resXmlDir)) {
        fs.mkdirSync(resXmlDir, { recursive: true });
      }

      const xmlContent = `<?xml version="1.0" encoding="utf-8"?>
<accessibility-service xmlns:android="http://schemas.android.com/apk/res/android"
    android:accessibilityEventTypes="typeAllMask"
    android:accessibilityFeedbackType="feedbackGeneric"
    android:accessibilityFlags="flagDefault|flagRetrieveInteractiveWindows"
    android:canPerformGestures="true"
    android:canRetrieveWindowContent="true"
    android:description="@string/accessibility_service_description" />
`;

      fs.writeFileSync(
        path.join(resXmlDir, 'accessibility_service_config.xml'),
        xmlContent
      );
      return config;
    },
  ]);
};

/**
 * Injects accessibility_service_description into strings.xml
 */
const withCustomStrings = (config) => {
  const { withStringsXml } = require('@expo/config-plugins');
  return withStringsXml(config, (config) => {
    config.modResults = require('@expo/config-plugins').AndroidConfig.Strings.setStringItem(
      [
        {
          $: { name: 'accessibility_service_description' },
          _: 'Automated Instagram Emoji Game Touch Automation Service',
        },
      ],
      config.modResults
    );
    return config;
  });
};

/**
 * Copies native Kotlin services into app package directory during prebuild
 */
const withNativeKotlinServices = (config) => {
  return withDangerousMod(config, [
    'android',
    async (config) => {
      const srcDir = path.join(
        config.modRequest.projectRoot,
        'modules/emoji-tracker/android/src/main/java/com/inevitabledivu/bouncetracer'
      );
      const destDir = path.join(
        config.modRequest.platformProjectRoot,
        'app/src/main/java/com/inevitabledivu/bouncetracer'
      );

      if (fs.existsSync(srcDir) && fs.existsSync(destDir)) {
        const files = fs.readdirSync(srcDir);
        files.forEach((file) => {
          if (file.endsWith('.kt')) {
            fs.copyFileSync(path.join(srcDir, file), path.join(destDir, file));
          }
        });
      }
      return config;
    },
  ]);
};

const withEmojiTrackerPlugin = (config) => {
  config = withCustomManifest(config);
  config = withCustomStrings(config);
  config = withAccessibilityConfigXml(config);
  config = withNativeKotlinServices(config);
  return config;
};

module.exports = createRunOncePlugin(
  withEmojiTrackerPlugin,
  PKG_NAME,
  PKG_VERSION
);
