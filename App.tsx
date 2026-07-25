import { StyleSheet, Text, View } from "react-native";
import { SafeAreaProvider, SafeAreaView } from "react-native-safe-area-context";
import { HUDOverlay } from "./src/components/HUDOverlay";

export default function App() {
    return (
        <SafeAreaProvider>
            <SafeAreaView style={styles.container}>
                <View style={styles.content}>
                    <Text style={styles.title}>
                        Bounce Tracker Control Panel
                    </Text>
                    <Text style={styles.subtitle}>
                        Automated Instagram Emoji Game Tracker & Touch
                        Dispatcher
                    </Text>
                </View>
                <HUDOverlay />
            </SafeAreaView>
        </SafeAreaProvider>
    );
}

const styles = StyleSheet.create({
    container: {
        flex: 1,
        backgroundColor: "#0f172a",
    },
    content: {
        flex: 1,
        justifyContent: "center",
        alignItems: "center",
        paddingHorizontal: 24,
    },
    title: {
        color: "#f8fafc",
        fontSize: 22,
        fontWeight: "bold",
        marginBottom: 8,
        textAlign: "center",
    },
    subtitle: {
        color: "#64748b",
        fontSize: 14,
        textAlign: "center",
        lineHeight: 20,
    },
});
