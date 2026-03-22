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
  side: string;
}

interface WorkoutContextType {
  workouts: CompletedWorkout[];
  saveWorkout: (workout: Omit<CompletedWorkout, 'id' | 'date'>) => Promise<void>;
  deleteAllWorkouts: () => Promise<void>;
  deleteWorkoutsByDateRange: (startDate: string, endDate: string) => Promise<void>;
  deleteWorkoutById: (id: string) => Promise<void>;
  isLoading: boolean;
}

const WorkoutContext = createContext<WorkoutContextType>({
  workouts: [],
  saveWorkout: async () => {},
  deleteAllWorkouts: async () => {},
  deleteWorkoutsByDateRange: async () => {},
  deleteWorkoutById: async () => {},
  isLoading: true,
});

export const WorkoutProvider = ({ children }: { children: React.ReactNode }) => {
  const [workouts, setWorkouts] = useState<CompletedWorkout[]>([]);
  const [isLoading, setIsLoading] = useState(true);

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

  const persist = async (updated: CompletedWorkout[]) => {
    setWorkouts(updated);
    await AsyncStorage.setItem(STORAGE_KEY, JSON.stringify(updated));
  };

  const saveWorkout = async (workout: Omit<CompletedWorkout, 'id' | 'date'>) => {
    try {
      const today = new Date();
      const date = today.toISOString().split('T')[0];
      const newWorkout: CompletedWorkout = {
        ...workout,
        id: Date.now().toString(),
        date,
      };
      await persist([newWorkout, ...workouts]);
    } catch (error) {
      console.error('Error saving workout:', error);
    }
  };

  const deleteAllWorkouts = async () => {
    try {
      await persist([]);
    } catch (error) {
      console.error('Error deleting all workouts:', error);
    }
  };

  const deleteWorkoutsByDateRange = async (startDate: string, endDate: string) => {
    try {
      const filtered = workouts.filter(
        w => w.date < startDate || w.date > endDate
      );
      await persist(filtered);
    } catch (error) {
      console.error('Error deleting workouts by date range:', error);
    }
  };

  const deleteWorkoutById = async (id: string) => {
    try {
      const filtered = workouts.filter(w => w.id !== id);
      await persist(filtered);
    } catch (error) {
      console.error('Error deleting workout:', error);
    }
  };

  return (
    <WorkoutContext.Provider
      value={{
        workouts,
        saveWorkout,
        deleteAllWorkouts,
        deleteWorkoutsByDateRange,
        deleteWorkoutById,
        isLoading,
      }}
    >
      {children}
    </WorkoutContext.Provider>
  );
};

export const useWorkouts = () => useContext(WorkoutContext);