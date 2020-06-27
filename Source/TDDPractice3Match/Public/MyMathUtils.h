// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
namespace MyMathUtils
{
    class TDDPRACTICE3MATCH_API Square {
    public:
        Square(FIntPoint leftLowerPoint, FIntPoint rightUpperPoint);
        bool Includes(FIntPoint point) const;
    private:
        int leftX = INT_MIN;
        int rightX = INT_MAX;
        int lowerY = INT_MIN;
        int upperY = INT_MAX;
    };
};