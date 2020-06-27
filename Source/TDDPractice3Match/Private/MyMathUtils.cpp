// Fill out your copyright notice in the Description page of Project Settings.


#include "MyMathUtils.h"

MyMathUtils::Square::Square(FIntPoint leftLowerPoint, FIntPoint rightUpperPoint)
	: leftX(leftLowerPoint.X), lowerY(leftLowerPoint.Y), rightX(rightUpperPoint.X), upperY(rightUpperPoint.Y)
{

}

bool MyMathUtils::Square::Includes(FIntPoint point) const
{
	return (point.X >= leftX) && (point.X <= rightX) &&
		(point.Y >= lowerY) && (point.Y <= upperY);
}
