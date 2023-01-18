//☀Rise☀
#include "minesweeper_ncomponent.h"

#include "grid_node.h"
#include "rectangle_node.h"
#include "margin_node.h"
#include "button_ncomponent.h"
#include "texture_ncomponent.h"
#include "label_ncomponent.h"
#include "9slice_ncomponent.h"

#include "resource_manager.h"

#include <cstdlib>

namespace Rise {

    REGISTER_NCOMPONENT(MinesweeperNComponent)

    MinesweeperNComponent::MinesweeperNComponent(Core* core, const Data& config)
        : NComponent(core) {
        _width = config["width"];
        _height = config["height"];
        _mines = config["mines"];

        _cellTexture = Rise::Instance()->ResourceGenerator().Get<Image>("cell");
        _markedTexture = Rise::Instance()->ResourceGenerator().Get<Image>("cell_marked");
        _mineTexture = Rise::Instance()->ResourceGenerator().Get<Image>("mine");
        _mineExplodedTexture = Rise::Instance()->ResourceGenerator().Get<Image>("mine_exploded");
        _mineCrossedTexture = Rise::Instance()->ResourceGenerator().Get<Image>("mine_marked");
        _emptyTexture = Rise::Instance()->ResourceGenerator().Get<Image>("empty");

        for (auto i = 0u; i < _numbersTextures.size(); ++i) {
            _numbersTextures[i] = Rise::Instance()->ResourceGenerator().Get<Image>("number_" + LexicalCast(i + 1));
        }
        std::array<std::shared_ptr<IImage>, 8> _numbers;

        uint32_t totalCount = _width * _height;

        time_t currentTime;
        std::time(&currentTime);
        std::srand(static_cast<unsigned int>(currentTime));

        _cells.resize(totalCount);
        for (auto i = 0u; i < _mines; ++i) {
            int32_t index;
            do {
                index = std::rand() % totalCount;
            } while (_cells[index].mineCount != 0);
            _cells[index].mineCount = 10;
        }

        for (auto index = 0u; index < totalCount; ++index) {
            if (_cells[index].mineCount > 0) {
                continue;
            }

            const auto x = index % _width;
            const auto y = index / _width;

            for (auto i = -1; i <= 1; ++i) {
                for (auto j = -1; j <= 1; ++j) {
                    const auto nx = x + i;
                    const auto ny = y + j;
                    if (nx >= _width || ny >= _height) {
                        continue;
                    }
                    const auto neighborIndex = _width * (y + j) + (x + i);
                    if (_cells[neighborIndex].mineCount == 10) {
                        ++_cells[index].mineCount;
                    }
                }
            }
        }
    }
    MinesweeperNComponent::~MinesweeperNComponent() {}

    void MinesweeperNComponent::leftClick(uint32_t x, uint32_t y) {
        if (_gameEnded) {
            return;
        }

        const auto index = _width * y + x;
        auto& cell = _cells[index];

        if (cell.open) {
            if (cell.mineCount == 0 || cell.mineCount == 10) {
                return;
            }

            auto count = 0u;

            for (auto i = -1; i <= 1; ++i) {
                for (auto j = -1; j <= 1; ++j) {
                    const auto nx = x + i;
                    const auto ny = y + j;
                    if (nx >= _width || ny >= _height) {
                        continue;
                    }

                    const auto nindex = _width * ny + nx;
                    if (_cells[nindex].marked) {
                        ++count;
                    }
                }
            }

            if (count != cell.mineCount) {
                return;
            }

            for (auto i = -1; i <= 1; ++i) {
                for (auto j = -1; j <= 1; ++j) {
                    const auto nx = x + i;
                    const auto ny = y + j;
                    if (nx >= _width || ny >= _height) {
                        continue;
                    }

                    const auto nindex = _width * ny + nx;
                    if (!_cells[nindex].marked) {
                        openCell(nx, ny);
                    }
                }
            }

            return;
        }

        openCell(x, y);
    }

    void MinesweeperNComponent::rightClick(uint32_t x, uint32_t y) {
        if (_gameEnded) {
            return;
        }

        const auto index = _width * y + x;
        auto& cell = _cells[index];

        if (cell.open) {
            if (cell.mineCount == 0 || cell.mineCount == 10) {
                return;
            }

            auto count = 0u;

            for (auto i = -1; i <= 1; ++i) {
                for (auto j = -1; j <= 1; ++j) {
                    const auto nx = x + i;
                    const auto ny = y + j;
                    if (nx >= _width || ny >= _height) {
                        continue;
                    }

                    const auto nindex = _width * ny + nx;
                    if (!_cells[nindex].open) {
                        ++count;
                    }
                }
            }

            if (count != cell.mineCount) {
                return;
            }

            for (auto i = -1; i <= 1; ++i) {
                for (auto j = -1; j <= 1; ++j) {
                    const auto nx = x + i;
                    const auto ny = y + j;
                    if (nx >= _width || ny >= _height) {
                        continue;
                    }

                    const auto nindex = _width * ny + nx;
                    if (!_cells[nindex].open && !_cells[nindex].marked) {
                        markCell(nx, ny);
                    }
                }
            }

            return;
        }

        markCell(x, y);
    }

    void MinesweeperNComponent::updateScore() {
        _markCountLabel->SetText(LexicalCast(static_cast<int32_t>(_mines - _marked)));
    }

    void MinesweeperNComponent::openCell(uint32_t x, uint32_t y) {
        const auto index = _width * y + x;
        auto& cell = _cells[index];

        if (cell.open) {
            return;
        }
        cell.open = true;

        if (cell.marked) {
            cell.marked = false;
            --_marked;
        }

        ++_opened;

        checkWin();

        auto textureCellComponent = _grid->Children()[index]->getComponent<TextureNComponent>();

        if (cell.mineCount == 10) {
            lose();

            textureCellComponent->setTexture(_mineExplodedTexture);
        }
        else if (cell.mineCount == 0) {
            textureCellComponent->setTexture(_emptyTexture);

            for (auto i = -1; i <= 1; ++i) {
                for (auto j = -1; j <= 1; ++j) {
                    const auto nx = x + i;
                    const auto ny = y + j;
                    if (nx >= _width || ny >= _height) {
                        continue;
                    }
                    openCell(nx, ny);
                }
            }
        }
        else {
            textureCellComponent->setTexture(_numbersTextures[cell.mineCount - 1]);
        }
    }

    void MinesweeperNComponent::markCell(uint32_t x, uint32_t y) {
        const auto index = _width * y + x;
        auto& cell = _cells[index];

        if (cell.open) {
            return;
        }

        cell.marked = !cell.marked;
        
        if (cell.marked) {
            ++_marked;
            if (cell.mineCount == 10) {
                ++_markedCorrect;
            }
        }
        else {
            --_marked;
            if (cell.mineCount == 10) {
                --_markedCorrect;
            }
        }

        checkWin();

        auto textureCellComponent = _grid->Children()[index]->getComponent<TextureNComponent>();

        textureCellComponent->setTexture(cell.marked ? _markedTexture : _cellTexture);
    }

    void MinesweeperNComponent::checkWin() {
        if (_marked == _markedCorrect && _marked == _mines) {
            win();
        }
        if (_opened == _cells.size() - _mines) {
            win();
        }
    }

    void MinesweeperNComponent::win() {
        if (_gameEnded) {
            return;
        }

        for (auto i = 0u; i < _cells.size(); ++i) {
            auto& cell = _cells[i];
            if (cell.open) {
                continue;
            }

            auto textureCellComponent = _grid->Children()[i]->getComponent<TextureNComponent>();

            if (cell.mineCount == 10) {
                textureCellComponent->setTexture(_markedTexture);
            }
            else if (cell.mineCount == 0) {
                textureCellComponent->setTexture(_emptyTexture);
            }
            else {
                textureCellComponent->setTexture(_numbersTextures[cell.mineCount - 1]);
            }
        }

        _gameEnded = true;
    }

    void MinesweeperNComponent::lose() {
        if (_gameEnded) {
            return;
        }

        for (auto i = 0u; i < _cells.size(); ++i) {
            auto& cell = _cells[i];
            if (cell.open) {
                continue;
            }

            auto textureCellComponent = _grid->Children()[i]->getComponent<TextureNComponent>();

            if (cell.mineCount == 10) {
                textureCellComponent->setTexture(cell.marked ? _markedTexture : _mineTexture);
            }
            else if (cell.marked) {
                textureCellComponent->setTexture(_mineCrossedTexture);
            }
        }

        _gameEnded = true;
    }

    void MinesweeperNComponent::onUpdateMetrics() {
        const float horizontalPixelCount = static_cast<float>(16 * _width + _n9slice->meta()->metrics.left + _n9slice->meta()->metrics.right);
        const auto& size = _n9sliceComponent->GetSize();

        const auto scale = size.width / horizontalPixelCount - 0.1f;
        
        _n9sliceComponent->setScale(scale);
        auto padding = _n9slice->meta()->metrics * scale;
        _marginNode->setMargin(padding);

        _markCountLabel->setSize(static_cast<uint32_t>(16 * scale));
    }

    void MinesweeperNComponent::onNodeSetted() {
        auto squareNode = Instance()->Resources().CreateRes<RectangleNode>(static_cast<float>(_width) / _height);
        node()->AddChild(squareNode);

        _n9sliceComponent = Instance()->Resources().CreateRes<N9SliceNComponent>();
        squareNode->addComponent(_n9sliceComponent);

        _marginNode = Instance()->Resources().CreateRes<MarginNode>();
        squareNode->AddChild(_marginNode);

        _n9slice = Instance()->ResourceGenerator().Get<N9Slice>("outer");
        _n9sliceComponent->setTexture(_n9slice);

        auto globalMenuNode = Instance()->Resources().CreateRes<GridNode>(1, 1);
        globalMenuNode->SetVerticalWeights({32.f, 4.f, 16.f * _width });
        _marginNode->AddChild(globalMenuNode);

        auto dummy = Instance()->Resources().CreateRes<Node>();

        _markCountLabel = Instance()->Resources().CreateRes<LabelNComponent>();
        _markCountLabel->SetFont(Instance()->ResourceGenerator().Get<Font>("times_new_roman"));
        updateScore();
        dummy->addComponent(_markCountLabel);

        auto dummy2 = Instance()->Resources().CreateRes<Node>();

        globalMenuNode->AddChild(dummy);
        globalMenuNode->AddChild(dummy2);

        auto gridNode = Instance()->Resources().CreateRes<GridNode>(_width, _height);
        _grid = gridNode.get();

        globalMenuNode->AddChild(gridNode);

        for (auto i = 0u; i < _cells.size(); ++i) {
            auto& cell = _cells[i];

            auto cellNode = Instance()->Resources().CreateRes<Node>();
            gridNode->AddChild(cellNode);

            auto textureCellComponent = Instance()->Resources().CreateRes<TextureNComponent>();
            textureCellComponent->setTexture(_cellTexture);
            cellNode->addComponent(textureCellComponent);

            auto buttonCellComponent = Instance()->Resources().CreateRes<ButtonNComponent>();
            buttonCellComponent->setAction([this, x = i % _width, y = i / _width](bool leftKey) {
                if (leftKey) {
                    leftClick(x, y); 
                }
                else {
                    rightClick(x, y);
                }
                updateScore();
                });
            cellNode->addComponent(buttonCellComponent);
        }
    }

}
