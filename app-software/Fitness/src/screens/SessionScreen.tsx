import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

const SessionScreen = () => {
  return (
    <View style={styles.container}>
      <View style={styles.header}>
        <Text style={styles.title}>Live Workout Session</Text>
        <Text style={styles.subtitle}>Ready to train?</Text>
      </View>
      
      <View style={styles.centerContent}>
        <View style={styles.timerContainer}>
          <Text style={styles.timer}>00:00</Text>
          <Text style={styles.timerLabel}>Duration</Text>
        </View>
        
        <TouchableOpacity style={styles.startButton}>
          <Text style={styles.startButtonText}>Start Workout</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: '#e0e0e0',
  },
  header: {
    padding: 20,
    paddingTop: 60,
    backgroundColor: '#e4e4e4',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    color: 'white',
  },
  subtitle: {
    fontSize: 16,
    color: '#aaa',
    marginTop: 5,
  },
  centerContent: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  timerContainer: {
    alignItems: 'center',
    marginBottom: 40,
  },
  timer: {
    fontSize: 72,
    fontWeight: 'bold',
    color: '#4CAF50',
  },
  timerLabel: {
    fontSize: 16,
    color: '#888',
    marginTop: 10,
  },
  startButton: {
    backgroundColor: '#4CAF50',
    paddingHorizontal: 40,
    paddingVertical: 15,
    borderRadius: 30,
    marginTop: 20,
  },
  startButtonText: {
    color: 'white',
    fontSize: 18,
    fontWeight: 'bold',
  },
});

export default SessionScreen;