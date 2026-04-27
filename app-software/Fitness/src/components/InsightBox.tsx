import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

type MessageType = 'warning' | 'success' | 'error';

interface InsightBoxProps {
  type: MessageType;
  message: string;
  bpm: number;
}

const InsightBox: React.FC<InsightBoxProps> = ({ type, message, bpm }) => {
  return (
    <View style={[styles.container, styles[type]]}>
      <View style={styles.textContainer}>
        <Text style={styles.text}>{message}</Text>
      </View>
      <View style={styles.bpmContainer}>
        <Text style={styles.bpmNumber}>{bpm}</Text>
        <Text style={styles.bpmLabel}>BPM</Text>
      </View>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    width: '85%',
    alignSelf: 'center',
    paddingVertical: 25,
    paddingHorizontal: 28,
    borderRadius: 8,
    borderColor: '#636363',
    borderWidth: 1,
    marginVertical: 10,
    flexDirection: 'row',
    alignItems: 'center',
  },
  textContainer: {
    flex: 1,
    paddingRight: 12,
  },
  text: {
    fontSize: 14,
    color: 'black',
    fontWeight: '500',
  },
  bpmContainer: {
    alignItems: 'center',
    justifyContent: 'center',
  },
  bpmNumber: {
    fontSize: 22,
    fontWeight: '700',
    color: 'black',
    opacity: 0.75,
  },
  bpmLabel: {
    fontSize: 11,
    fontWeight: '600',
    color: 'black',
    opacity: 0.75,
    letterSpacing: 1,
  },
  warning: {
    backgroundColor: '#F1E5C4',
  },
  success: {
    backgroundColor: '#D6E0D3',
  },
  error: {
    backgroundColor: '#DCBDBD',
  },
});

export default InsightBox;