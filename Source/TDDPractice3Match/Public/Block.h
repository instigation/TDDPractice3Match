#pragma once

#include "CoreMinimal.h"
#include "MatchRules.h"

/**
 *
 */
enum class TDDPRACTICE3MATCH_API Block {
    MIN,
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    MAX_NORMAL,
    MAX_SPECIAL,
    INVALID,
    MAX
};

TArray<Block> GetNormalBlocks();

enum class TDDPRACTICE3MATCH_API BlockColor {
    ZERO,
    ONE,
    TWO,
    THREE,
    FOUR,
    MAX
};

static BlockColor GetColor(Block block);

class TDDPRACTICE3MATCH_API BlockMatrix {
public:
    BlockMatrix() {}
    BlockMatrix(int numRows, int numCols, const TMap<FIntPoint, Block> blockMatrix);
    BlockMatrix(const TArray<TArray<Block>>& block2DArray);
    TArray<TArray<Block>> GetBlock2DArray() const { return block2DArray; }
    bool HasNoMatch() const;
    Block At(int row, int col) const;
    void ProcessMatch();
    int GetNumRows() const { return numRows; }
    int GetNumCols() const { return numCols; }
private:
    static int GetRow(FIntPoint point) { return point.X; }
    static int GetCol(FIntPoint point) { return point.Y; }
    bool IsOutOfMatrix(FIntPoint point) const;
    TArray<TPair<FIntPoint, Formation>> GetMatchedLocationAndFormations() const;
    int numRows = 0;
    int numCols = 0;
    TMap<FIntPoint, Block> blockMatrix;
    TArray<TArray<Block>> block2DArray;
};