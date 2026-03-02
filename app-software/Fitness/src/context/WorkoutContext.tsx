import React, { createContext, useContext, useState } from 'react';

export interface CompletedSet {
  setNumber: number;
  repsCompleted: number;
}

export interface CompletedWorkout {
  id: string;
  date: string;        // YYYY-MM-DD
  exercise: string;
  sets: CompletedSet[];
  totalReps: number;
}

interface WorkoutContextType {
  workouts: CompletedWorkout[];
  saveWorkout: (workout: Omit<CompletedWorkout, 'id' | 'date'>) => void;
}

const WorkoutContext = createContext<WorkoutContextType>({
  workouts: [],
  saveWorkout: () => {},
});

export const WorkoutProvider = ({ children }: { children: React.ReactNode }) => {
  const [workouts, setWorkouts] = useState<CompletedWorkout[]>([]);

  const saveWorkout = (workout: Omit<CompletedWorkout, 'id' | 'date'>) => {
    const today = new Date();
    const date = today.toISOString().split('T')[0]; // YYYY-MM-DD
    const newWorkout: CompletedWorkout = {
      ...workout,
      id: Date.now().toString(),
      date,
    };
    setWorkouts(prev => [newWorkout, ...prev]);
  };

  return (
    <WorkoutContext.Provider value={{ workouts, saveWorkout }}>
      {children}
    </WorkoutContext.Provider>
  );
};

export const useWorkouts = () => useContext(WorkoutContext);