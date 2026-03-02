/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 */

import React from 'react';
import { NavigationContainer } from '@react-navigation/native';
import { SafeAreaProvider } from 'react-native-safe-area-context';
import { StatusBar } from 'react-native';
import { WorkoutProvider } from './src/context/WorkoutContext';
import RootNavigator from './src/navigation/RootNavigator';

const App = () => (
  <WorkoutProvider>
    <SafeAreaProvider>
      <StatusBar barStyle="dark-content" backgroundColor="#ffffff" />
      <NavigationContainer>
        <RootNavigator /> 
      </NavigationContainer>
    </SafeAreaProvider>
  </WorkoutProvider>
);

export default App;