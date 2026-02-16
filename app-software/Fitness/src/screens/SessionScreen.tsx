import React from 'react';
import { View, Text, StyleSheet, TouchableOpacity } from 'react-native';

const SessionScreen = () => {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Live Session</Text>
      
      <View style={styles.centerContent}>
        <TouchableOpacity style={styles.startButton}>
          <Text style={styles.startButtonText}>Start a workout</Text>
        </TouchableOpacity>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    flex: 1,
    backgroundColor: 'white',
  },
  title: {
    fontSize: 24,
    fontWeight: 'bold',
    padding: 20,
    paddingTop: 65,
    backgroundColor: 'white',
  },
  centerContent: {
    flex: 1,
    justifyContent: 'center',
    alignItems: 'center',
  },
  startButton: {
    backgroundColor: '#D6E0D3',
    paddingHorizontal: 40,
    paddingVertical: 15,
    borderRadius: 30,
    marginTop: 20,
    borderWidth: 1,
    borderColor: '#636363',
  },
  startButtonText: {
    color: 'black',
    fontSize: 18,
    fontWeight: '500',
  },
});

export default SessionScreen;