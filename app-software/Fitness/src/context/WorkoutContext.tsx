import React, { createContext, useContext, useState, useEffect } from 'react';
import AsyncStorage from '@react-native-async-storage/async-storage';

const STORAGE_KEY = 'workouts';

export interface CompletedSet {
  setNumber: number;
  repsCompleted: number;
}

export interface CompletedWorkout {
  id: string;
  date: string; // YYYY-MM-DD
  exercise: string;
  sets: CompletedSet[];
  totalReps: number;
}

interface WorkoutContextType {
  workouts: CompletedWorkout[];
  saveWorkout: (workout: Omit<CompletedWorkout, 'id' | 'date'>) => Promise<void>;
  isLoading: boolean;
}

const WorkoutContext = createContext<WorkoutContextType>({
  workouts: [],
  saveWorkout: async () => {},
  isLoading: true,
});

export const WorkoutProvider = ({ children }: { children: React.ReactNode }) => {
  const [workouts, setWorkouts] = useState<CompletedWorkout[]>([]);
  const [isLoading, setIsLoading] = useState(true);

  // Load workouts from AsyncStorage on mount
  useEffect(() => {
    const loadWorkouts = async () => {
      try {
        const stored = await AsyncStorage.getItem(STORAGE_KEY);
        if (stored) {
          setWorkouts(JSON.parse(stored));
        }
      } catch (error) {
        console.error('Error loading workouts:', error);
      } finally {
        setIsLoading(false);
      }
    };

    loadWorkouts();
  }, []);

  const saveWorkout = async (workout: Omit<CompletedWorkout, 'id' | 'date'>) => {
    try {
      const today = new Date();
      const date = today.toISOString().split('T')[0]; // YYYY-MM-DD
      const newWorkout: CompletedWorkout = {
        ...workout,
        id: Date.now().toString(),
        date,
      };

      const updatedWorkouts = [newWorkout, ...workouts];
      setWorkouts(updatedWorkouts);
      await AsyncStorage.setItem(STORAGE_KEY, JSON.stringify(updatedWorkouts));
    } catch (error) {
      console.error('Error saving workout:', error);
    }
  };

  return (
    <WorkoutContext.Provider value={{ workouts, saveWorkout, isLoading }}>
      {children}
    </WorkoutContext.Provider>
  );
};

export const useWorkouts = () => useContext(WorkoutContext);