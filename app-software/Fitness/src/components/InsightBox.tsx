import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

type MessageType = 'warning' | 'success' | 'error';

interface InsightBoxProps {
  type: MessageType;
  message: string;
}

// TODO: add icons for each type
const InsightBox: React.FC<InsightBoxProps> = ({ type, message }) => {
  return (
    <View style={[styles.container, styles[type]]}>
      <Text style={styles.text}>{message}</Text>
    </View>
  );
};

const styles = StyleSheet.create({
  container: {
    width: '85%', 
    alignSelf: 'center',
    paddingVertical: 30,
    paddingHorizontal: 28,
    borderRadius: 8,
    borderColor: '#636363',
    borderWidth: 1,
    marginVertical: 10,
  },
  text: {
    fontSize: 14,
    color: 'black',
    fontWeight: '500',
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
