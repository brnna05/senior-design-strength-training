import React from 'react';
import { View, Text, StyleSheet, ScrollView } from 'react-native';

const HistoryScreen = () => {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Workout History</Text>
      <ScrollView style={styles.content}>
        <Text style={styles.placeholder}>
          TODO: Add calendar, insights, and achievements here.
        </Text>
      </ScrollView>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#f5f5f5',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    padding: 20,
    paddingTop: 60,
    backgroundColor: 'white',
  },
  content: {
    flex: 1,
    padding: 20,
  },
  placeholder: {
    fontSize: 16,
    color: '#666',
    textAlign: 'center',
    marginTop: 50,
  },
});

export default HistoryScreen;