//☀Rise☀
#ifndef minesweeper_ncomponent_h
#define minesweeper_ncomponent_h

#include "node.h"

#include "image.h"

namespace Rise {

    class N9SliceNComponent;
    class N9Slice;
    class MarginNode;
    class LabelNComponent;

    class MinesweeperNComponent : public NComponent {
    public:
        explicit MinesweeperNComponent(Core* core, const Data& config);
        ~MinesweeperNComponent() override;

    private:

        void leftClick(uint32_t x, uint32_t y);
        void rightClick(uint32_t x, uint32_t y);

        void openCell(uint32_t x, uint32_t y);
        void markCell(uint32_t x, uint32_t y);

        void checkWin();
        void win();
        void lose();

        void updateScore();

        void onNodeSetted() override;
        void onUpdateMetrics() override;

        struct Cell {
            uint32_t mineCount = 0; // 0-9: mines around 10: its a mine
            bool open = false;
            bool marked = false;
        };

        uint32_t _width;
        uint32_t _height;
        uint32_t _mines;

        uint32_t _opened = 0;
        uint32_t _marked = 0;
        uint32_t _markedCorrect = 0;

        std::vector<Cell> _cells;
        Node* _grid = nullptr;

        std::shared_ptr<N9SliceNComponent> _n9sliceComponent;
        std::shared_ptr<N9Slice> _n9slice;
        std::shared_ptr<MarginNode> _marginNode;
        std::shared_ptr<LabelNComponent> _markCountLabel;

        bool _gameEnded = false;

        std::shared_ptr<IImage> _cellTexture;
        std::shared_ptr<IImage> _markedTexture;
        std::shared_ptr<IImage> _mineTexture;
        std::shared_ptr<IImage> _mineExplodedTexture;
        std::shared_ptr<IImage> _mineCrossedTexture;
        std::shared_ptr<IImage> _emptyTexture;
        std::array<std::shared_ptr<IImage>, 8> _numbersTextures;
    };

}

#endif /* minesweeper_ncomponent_h */
