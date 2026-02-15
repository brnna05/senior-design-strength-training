import React from 'react';
import { createBottomTabNavigator } from '@react-navigation/bottom-tabs';

// Screens
import HistoryScreen from '../screens/HistoryScreen';
import PlannerScreen from '../screens/PlannerScreen';
import SessionScreen from '../screens/SessionScreen';
import SettingsScreen from '../screens/SettingsScreen';

// Logos
import HistoryIcon from '../assets/icons/history.svg';
import PlannerIcon from '../assets/icons/planner.svg';
import SessionIcon from '../assets/icons/session.svg';
import SettingsIcon from '../assets/icons/settings.svg';

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
          fontSize: 10,
          fontWeight: '500',
        },
        headerShown: false,
      }}
    >
      <Tab.Screen
        name="History"
        component={HistoryScreen}
        options={{
        tabBarIcon: ({ color, size }) => (
          <HistoryIcon width={size} height={size} fill={color} />
        ),
        }}
      />

      <Tab.Screen
        name="Planner"
        component={PlannerScreen}
        options={{ tabBarIcon: ({ color, size }) => (
          <PlannerIcon width={size} height={size} fill={color} />
          ),
        }}
      />

      <Tab.Screen
        name="Session"
        component={SessionScreen}
        options={{ tabBarIcon: ({ color, size }) => (
          <SessionIcon width={size} height={size} fill={color} />
          ),
        }}
      />
      <Tab.Screen
        name="Settings"
        component={SettingsScreen}
        options={{ tabBarIcon: ({ color, size }) => (
          <SettingsIcon width={size} height={size} fill={color} />
          ),
        }}
      />
    </Tab.Navigator>
  );
};

export default BottomTabNavigator;
