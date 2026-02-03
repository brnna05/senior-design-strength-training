import React from 'react';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';

// Import screens
import HistoryScreen from '../screens/HistoryScreen';
import PlannerScreen from '../screens/PlannerScreen';
import SessionScreen from '../screens/SessionScreen';
import SettingsScreen from '../screens/SettingsScreen';

// Logos
import HistoryLogo from '../assets/icons/leaderboard.svg';
import PlannerLogo from '../assets/icons/event_note.svg';
import SessionLogo from '../assets/icons/exercise.svg';
import SettingsLogo from '../assets/icons/settings.svg';

const Tab = createBottomTabNavigator();

const BottomTabNavigator = () => {
  return (
    <Tab.Navigator
      screenOptions={{
        tabBarActiveTintColor: '#000000',
        tabBarInactiveTintColor: 'gray',
        tabBarStyle: {
          backgroundColor: 'white',
          borderTopWidth: 1,
          borderTopColor: '#e0e0e0',
          height: 60,
          paddingBottom: 10,
          paddingTop: 5,
          marginBottom: 20,
        },
        tabBarLabelStyle: {
          fontSize: 14,
          fontWeight: '500',
        },
        headerShown: false,
      }}
    >
      <Tab.Screen
        name="History"
        component={HistoryScreen}
        options={{ title: 'History' }}
      />
      <Tab.Screen
        name="Planner"
        component={PlannerScreen}
        options={{ title: 'Planner' }}
      />
      <Tab.Screen
        name="Session"
        component={SessionScreen}
        options={{ title: 'Session' }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ title: 'Settings' }}
      />
    </Tab.Navigator>
  );
};

export default BottomTabNavigator;
