import React from 'react';
import { View, Text, StyleSheet } from 'react-native';

type MessageType = 'warning' | 'success' | 'error';

import WarningIcon from '../assets/icons/warning.svg';
import SuccessIcon from '../assets/icons/thumb_up.svg';
import ErrorIcon from '../assets/icons/thumb_down.svg';

const iconMap = {
  warning: WarningIcon,
  success: SuccessIcon,
  error: ErrorIcon,
} as const;


interface InsightBoxProps {
  type: MessageType;
  message: string;
}

// TODO: add icons for each type
const InsightBox: React.FC<InsightBoxProps> = ({ type, message }) => {
  const Icon = iconMap[type];

  return (
    <View style={[styles.container, styles[type]]}>
      <View style={styles.textContainer}>
        <Text style={styles.text}>{message}</Text>
      </View>

      <Icon width={40} height={40} color={'black'} opacity={0.75}/>
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
