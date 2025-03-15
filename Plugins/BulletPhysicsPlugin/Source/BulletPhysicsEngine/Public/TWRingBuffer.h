#pragma once

#include "CoreMinimal.h"

/**
 * Ring buffer data structure with configurable capacity
 * using TArray with minimal memory allocation.
 */
template<typename T>
class TWRingBuffer
{
public:
    /** Constructor with configurable capacity */
    explicit TWRingBuffer(int32 InCapacity = 128)
        : Capacity(InCapacity)
        , Size(0)
        , Head(0)
    {
        // No pre-allocation, we'll grow as needed
    }

    /** Add an element to the buffer */
    void Push(const T& Item)
    {
        // Make sure the array is big enough for the current head position
        if (Buffer.Num() <= Head)
        {
            Buffer.AddDefaulted(1); // Just add what we need
        }
        
        Buffer[Head] = Item;
        Head = (Head + 1) % Capacity;
        
        // If we've wrapped around and need to overwrite, ensure the array is large enough
        if (Head < Buffer.Num() || Buffer.Num() >= Capacity)
        {
            // Array already has enough elements
        }
        else
        {
            // Need to expand the array to accommodate the next insertion
            Buffer.AddDefaulted(1);
        }
        
        if (Size < Capacity) { Size++; }
    }

    /** Get item at position (0 is most recent, 1 is one before that, etc.) */
    T Get(int32 Position) const
    {
        if (Position >= Size || Position < 0 || Buffer.Num() == 0)
        {
            UE_LOG(LogTemp, Error, TEXT("TWRingBuffer::Get: Invalid Position"));
            return T();
        }
            
        int32 Index = (Head - 1 - Position);
        if (Index < 0)
        {
            Index += Capacity;
        }
        
        // Check if the calculated index exists in the array
        if (Index >= Buffer.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("TWRingBuffer::Get: Index out of range"));
            return T();
        }
        
        return Buffer[Index];
    }

    /** Get the oldest item in the buffer */
    T GetOldest() const
    {
        if (Size == 0 || Buffer.Num() == 0)
        {
            return T();
        }
            
        int32 OldestIndex = (Head - Size + Capacity) % Capacity;
        
        // Check if the calculated index exists in the array
        if (OldestIndex >= Buffer.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("TWRingBuffer::GetOldest: Index out of range"));
            return T();
        }
        
        return Buffer[OldestIndex];
    }

    /** Get the newest item in the buffer */
    T GetNewest() const
    {
        if (Size == 0 || Buffer.Num() == 0)
        {
            return T();
        }
            
        int32 NewestIndex = (Head - 1 + Capacity) % Capacity;
        
        // Check if the calculated index exists in the array
        if (NewestIndex >= Buffer.Num())
        {
            UE_LOG(LogTemp, Error, TEXT("TWRingBuffer::GetNewest: Index out of range"));
            return T();
        }
        
        return Buffer[NewestIndex];
    }

    /** Return number of items currently in the buffer */
    int32 GetSize() const
    {
        return Size;
    }

    /** Check if the buffer is empty */
    bool IsEmpty() const
    {
        return Size == 0;
    }

    /** Check if the buffer is full */
    bool IsFull() const
    {
        return Size == Capacity;
    }

    /** Clear the buffer */
    void Clear()
    {
        Buffer.Empty(); // Release all memory
        Size = 0;
        Head = 0;
    }

private:
    TArray<T> Buffer;  // TArray to store the data
    int32 Capacity;    // Maximum capacity
    int32 Size;        // Current number of elements
    int32 Head;        // Position to write the next element
};