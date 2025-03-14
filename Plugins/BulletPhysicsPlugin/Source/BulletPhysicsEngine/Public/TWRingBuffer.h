#pragma once

#include "CoreMinimal.h"

/**
 * Ring buffer data structure that can hold up to 64 entries
 * and allows rewinding to read previous entries.
 */
template<typename T>
class TWRingBuffer
{
public:
    /** Constructor - initialize with capacity of 64 */
    TWRingBuffer()
        : Buffer()
        , Capacity(128)
        , Size(0)
        , Head(0)
    {
    }

    /** Add an element to the buffer */
    void Push(const T& Item)
    {
        Buffer[Head] = Item;
        Head = (Head + 1) % Capacity;
        if (Size < Capacity) { Size++; }
     //    UE_LOG(LogTemp, Warning, TEXT("Pushed, head: %i"), Head);
	    // GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("Pushed, head: %i"), Head));
    }

    /** Get item at position (0 is most recent, 1 is one before that, etc.) */
    T Get(int32 Position) const
    {
        if (Position >= Size || Position < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("TWRingBuffer::Get: Invalid Position"));
            GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("BAD POSITION: %i"), Head));
            return T();
        }
            
        int32 Index = (Head - 1 - Position);
        if (Index < 0)
        {
            Index += Capacity;
        }
        // UE_LOG(LogTemp, Warning, TEXT("GOT: %i"),Index );
        // GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Yellow, FString::Printf(TEXT("GOT: %i"), Index));
        return Buffer[Index];
    }

    /** Get the oldest item in the buffer */
    T GetOldest() const
    {
        if (Size == 0)
        {
            return T();
        }
            
        int32 OldestIndex = (Head - Size + Capacity) % Capacity;
        return Buffer[OldestIndex];
    }

    /** Get the newest item in the buffer */
    T GetNewest() const
    {
        if (Size == 0)
        {
            return T();
        }
            
        int32 NewestIndex = (Head - 1 + Capacity) % Capacity;
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
        Size = 0;
        Head = 0;
    }

private:
    T Buffer[128];      // Array to store the data
    int32 Capacity;    // Fixed capacity (128)
    int32 Size;        // Current number of elements
    int32 Head;        // Position to write the next element
};