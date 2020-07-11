// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

class ExplosionArea {
public:
	virtual ~ExplosionArea() {}
	virtual bool Contains(const FVector2D& position) const = 0;
};

class VerticalLineExplosionArea : public ExplosionArea {
public:
	VerticalLineExplosionArea(const FVector2D& centerPosition, float gridSize)
		: minY(centerPosition.Y - gridSize / 2), maxY(centerPosition.Y + gridSize / 2) {}
	virtual bool Contains(const FVector2D& position) const override { return (minY <= position.Y) && (position.Y <= maxY); }
private:
	float minY;
	float maxY;
};

class HorizontalLineExplosionArea : public ExplosionArea {
public:
	HorizontalLineExplosionArea(const FVector2D& centerPosition, float gridSize)
		: minX(centerPosition.X - gridSize / 2), maxX(centerPosition.X + gridSize / 2) {}
	virtual bool Contains(const FVector2D& position) const override { return (minX <= position.X) && (position.X <= maxX); }
private:
	float minX;
	float maxX;
};

class EmptyExplosionArea : public ExplosionArea {
public:
	virtual bool Contains(const FVector2D& position) const override { return false; }
};