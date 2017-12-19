//
//  main.cpp
//  cube
//
//  Created by Vilhelm Hedberg on 12/14/17.
//  Copyright © 2017 Apple. All rights reserved.
//

#include <iostream>
#include <array>
#include <vector>
#include <unordered_map>
#include <string>
#include <sstream>
#include <algorithm>
#include <iterator>
#include <random>


// Functional utilities

template<typename... Args>
bool all(Args... args) { return (... && args); }

// Axes:
// X -> Right, Y -> Up, Z -> Forward
enum class Axis : uint8_t {
    X,
    Y,
    Z
};

enum class Color : uint8_t {
    White = 0,
    Orange,
    Green,
    Red,
    Blue,
    Yellow,
    None
};

constexpr const char* to_string(const Color& c)
{
    switch (c) {
        case Color::White:
            return "W";
        case Color::Orange:
            return "O";
        case Color::Green:
            return "G";
        case Color::Red:
            return "R";
        case Color::Blue:
            return "B";
        case Color::Yellow:
            return "Y";
        case Color::None:
            return "-";
    }
}

enum class Face : uint8_t {
    Top = 0,
    Left,
    Front,
    Right,
    Back,
    Bottom
};

struct Layer {
    constexpr Layer(Axis a, size_t i) : axis(a), index(i) {}
    Axis axis;
    size_t index;
};

constexpr Layer layer_left(const size_t cubeSize) { return Layer(Axis::X, 0); }
constexpr Layer layer_middle(const size_t cubeSize) { return Layer(Axis::X, cubeSize / 2); }
constexpr Layer layer_right(const size_t cubeSize) { return Layer(Axis::X, cubeSize - 1); }
constexpr Layer layer_bottom(const size_t cubeSize) { return Layer(Axis::Y, 0); }
constexpr Layer layer_equator(const size_t cubeSize) { return Layer(Axis::Y, cubeSize / 2); }
constexpr Layer layer_top(const size_t cubeSize) { return Layer(Axis::Y, cubeSize - 1); }
constexpr Layer layer_front(const size_t cubeSize) { return Layer(Axis::Z, 0); }
constexpr Layer layer_slice(const size_t cubeSize) { return Layer(Axis::Z, cubeSize / 2); }
constexpr Layer layer_back(const size_t cubeSize) { return Layer(Axis::Z, cubeSize - 1); }

constexpr Layer layer(Face face, const size_t cubeSize) {
    switch (face) {
        case Face::Top:
            return layer_top(cubeSize);
        case Face::Left:
            return layer_left(cubeSize);
        case Face::Front:
            return layer_front(cubeSize);
        case Face::Right:
            return layer_right(cubeSize);
        case Face::Back:
            return layer_back(cubeSize);
        case Face::Bottom:
            return layer_bottom(cubeSize);
    }
}

constexpr Color default_color(Face f)
{
    switch (f) {
        case Face::Top:
            return Color::White;
        case Face::Left:
            return Color::Orange;
        case Face::Front:
            return Color::Green;
        case Face::Right:
            return Color::Red;
        case Face::Back:
            return Color::Blue;
        case Face::Bottom:
            return Color::Yellow;
    }
}

typedef std::array<Color, 6> Piece;

namespace piece {
    
    Piece default_piece()
    {
        Piece p;
        p.fill(Color::None);
        return p;
    }
    
    Piece default_face(Face f)
    {
        Piece p = default_piece();
        p[static_cast<size_t>(f)] = default_color(f);
        return p;
    }
    
    Piece add_default_face(Piece&& p, Face f)
    {
        p[static_cast<size_t>(f)] = default_color(f);
        return p;
    }
    
    constexpr bool has(const Piece& p, Face f)
    {
        return p[static_cast<size_t>(f)] != Color::None;
    }
    
    Piece shift(Piece&& p, std::array<Face, 4>&& faceIndices, bool clockwise)
    {
        Color prev;
        if (!clockwise) {
            for (auto it = faceIndices.begin(); it != faceIndices.end(); it++) {
                std::swap(p[static_cast<size_t>(*it)], prev);
            }
            std::swap(p[static_cast<size_t>(faceIndices.front())], prev);
        } else {
            for (auto it = faceIndices.rbegin(); it != faceIndices.rend(); it++) {
                std::swap(p[static_cast<size_t>(*it)], prev);
            }
            std::swap(p[static_cast<size_t>(faceIndices.back())], prev);
        }
        return p;
    }
    
    Piece rotate(Piece&&p, Axis axis, bool clockwise)
    {
        switch (axis) {
            case Axis::X:
                return shift(std::move(p), {Face::Top, Face::Back, Face::Bottom, Face::Front}, clockwise);
            case Axis::Y:
                return shift(std::move(p), {Face::Front, Face::Left, Face::Back, Face::Right}, clockwise);
            case Axis::Z:
                return shift(std::move(p), {Face::Top, Face::Left, Face::Bottom, Face::Right}, clockwise);
        }
    }
    
    std::ostream& print_face(const Piece& p, Face face, std::ostream& os)
    {
        return os << to_string(p[static_cast<size_t>(face)]);
    }
}


typedef std::vector<Piece> Cube;


namespace cube {
    
    struct Coord
    {
        size_t x, y, z;
    };
    
    constexpr size_t index(size_t cubeSize, size_t x, size_t y, size_t z)
    {
        return (z * cubeSize + y) * cubeSize + x;
    }
    
    constexpr size_t index(size_t cubeSize, const Coord& c)
    {
        return index(cubeSize, c.x, c.y, c.z);
    }
    
    typedef std::function<Coord(size_t, size_t)> PlaneToCubeMapping;
    
    PlaneToCubeMapping planeToCube(size_t cubeSize, Layer l)
    {
        auto planeToCube = [cubeSize](size_t begin, int xStride, int yStride) -> PlaneToCubeMapping {
            return [begin, xStride, yStride, cubeSize](size_t lx, size_t ly) -> Coord {
                size_t idx = begin + (ly*yStride) + lx*xStride;
                size_t z = idx / (cubeSize * cubeSize);
                size_t layerIdx = idx - z * cubeSize * cubeSize;
                size_t y = layerIdx / cubeSize;
                size_t x = layerIdx % cubeSize;
                return {.x = x, .y = y, .z = z};
            };
        };
        
        auto isFirstHalf = [cubeSize](size_t index) -> bool { return index <= cubeSize / 2; };
        auto inverseIndex = [cubeSize](size_t index) -> size_t { return cubeSize - 1 - index; };
        
        switch (l.axis) {
            case Axis::X:
                return planeToCube(isFirstHalf(l.index) ? index(cubeSize, l.index, 0, 0) : index(cubeSize, cubeSize - 1 - inverseIndex(l.index), cubeSize - 1, 0),
                                   (isFirstHalf(l.index) ? 1 : -1) * static_cast<int>(cubeSize),
                                   cubeSize * cubeSize);
            case Axis::Y:
                return planeToCube(isFirstHalf(l.index) ? index(cubeSize, 0, cubeSize - 1, cubeSize - 1 - l.index) : index(cubeSize, 0, 0, cubeSize - 1 - l.index),
                                   1,
                                   (isFirstHalf(l.index) ? -1 : 1) * static_cast<int>(cubeSize));
            case Axis::Z:
                return planeToCube(isFirstHalf(l.index) ? index(cubeSize, 0, cubeSize - 1 - l.index, 0) : index(cubeSize, cubeSize - 1, cubeSize - 1 - l.index, 0),
                                   isFirstHalf(l.index) ? 1 : -1,
                                   cubeSize * cubeSize);
        }
    }
    
    Cube default_cube(size_t cubeSize)
    {
        typedef typename Cube::iterator CubeIterator;
        auto createRow = [cubeSize](CubeIterator rowBegin, bool hasTopOrBottom, Face topOrBottom, bool hasFrontOrBack, Face frontOrBack) {
            // Left
            *rowBegin = piece::default_face(Face::Left);
            // Right
            *(rowBegin + cubeSize - 1) = piece::default_face(Face::Right);
            
            if (hasFrontOrBack) {
                // Left
                *rowBegin = piece::add_default_face(std::move(*rowBegin), frontOrBack);
                // Middle
                for (size_t i = 1; i < cubeSize - 1; i++) {
                    *(rowBegin + i) = piece::add_default_face(std::move(*(rowBegin + i)), frontOrBack);
                }
                // Right
                *(rowBegin + cubeSize - 1) = piece::add_default_face(std::move(*(rowBegin + cubeSize - 1)), frontOrBack);
            }
            
            if (hasTopOrBottom) {
                for (size_t i = 0; i < cubeSize; i++) {
                    *(rowBegin + i) = piece::add_default_face(std::move(*(rowBegin + i)), topOrBottom);
                }
            }
        };
        
        auto rowIterator = [cubeSize](Cube& c, size_t y, size_t z) -> CubeIterator {
            return c.begin() + index(cubeSize, 0, y, z);
        };
        
        auto createLayer = [createRow, rowIterator, cubeSize](Cube& cube, size_t z, bool hasTopOrBottom, Face topOrBottom) {
            createRow(rowIterator(cube, 0, z), hasTopOrBottom, topOrBottom, true, Face::Back);
            for (size_t y = 1; y < cubeSize - 1; y++) {
                createRow(rowIterator(cube, y, z), hasTopOrBottom, topOrBottom, false, Face::Back);
            }
            createRow(rowIterator(cube, cubeSize - 1, z), hasTopOrBottom, topOrBottom, true, Face::Front);
            
        };
        
        Cube cube(cubeSize*cubeSize*cubeSize, piece::default_piece());
        
        // Top layer
        createLayer(cube, 0, true, Face::Top);
        // Middle layers
        for (size_t z = 1; z < cubeSize - 1; z++) {
            createLayer(cube, z, false, Face::Top);
        }
        // Bottom layer
        createLayer(cube, cubeSize - 1, true, Face::Bottom);
        
        return cube;
    }
    
    bool validate(size_t cubeSize, const Cube& cube)
    {
        auto validateSide = [&cube, cubeSize](PlaneToCubeMapping plane2Cube, Face face) -> bool {
            for (size_t y = 0; y < cubeSize; y++) {
                for (size_t x = 0; x < cubeSize; x++) {
                    const Piece& p = cube[index(cubeSize, plane2Cube(x, y))];
                    if (p[static_cast<size_t>(face)] != default_color(face)) {
                        return false;
                    }
                }
            }
            return true;
        };
        
        return all(validateSide(planeToCube(cubeSize, layer(Face::Top, cubeSize)), Face::Top),
                   validateSide(planeToCube(cubeSize, layer(Face::Left, cubeSize)), Face::Left),
                   validateSide(planeToCube(cubeSize, layer(Face::Front, cubeSize)), Face::Front),
                   validateSide(planeToCube(cubeSize, layer(Face::Right, cubeSize)), Face::Right),
                   validateSide(planeToCube(cubeSize, layer(Face::Back, cubeSize)), Face::Back),
                   validateSide(planeToCube(cubeSize, layer(Face::Bottom, cubeSize)), Face::Bottom));
    }
    
    Cube rotate(size_t cubeSize, Cube&& cube, Layer layer, bool clockwise)
    {
        // 1. First rotate the layer
        PlaneToCubeMapping mapping = planeToCube(cubeSize, layer);
        //   1.a First flip
        if (clockwise) {
            // Flip vertically
            for (size_t y = 0; y < cubeSize/2; y++) {
                for (size_t x = 0; x < cubeSize; x++) {
                    std::swap(cube[index(cubeSize, mapping(x, y))], cube[index(cubeSize, mapping(x, cubeSize - 1 - y))]);
                }
            }
        } else {
            // Flip horizontally
            for (size_t y = 0; y < cubeSize; y++) {
                for (size_t x = 0; x < cubeSize/2; x++) {
                    std::swap(cube[index(cubeSize, mapping(x, y))], cube[index(cubeSize, mapping(cubeSize - 1 - x, y))]);
                }
            }
        }
        
        //   1.b Transpose
        for (size_t y = 0; y < cubeSize - 1; y++) {
            for (size_t x = y + 1; x < cubeSize; x++) {
                std::swap(cube[index(cubeSize, mapping(x, y))], cube[index(cubeSize, mapping(y, x))]);
            }
        }
        
        // 2. Then rotate all the pieces in the layer
        const bool clockwisePiece = (layer.index <= cubeSize/2);
        for (size_t y = 0; y < cubeSize; y++) {
            for (size_t x = 0; x < cubeSize; x++) {
                cube[index(cubeSize, mapping(x, y))] = piece::rotate(std::move(cube[index(cubeSize, mapping(x, y))]), layer.axis, clockwisePiece ? clockwise : !clockwise);
            }
        }
        
        return cube;
    }

    std::ostream& print(size_t cubeSize, const Cube& cube, std::ostream& os)
    {
        auto printRow = [cubeSize](const Cube& cube, std::ostream& os, size_t spaces, size_t row, Face face) -> std::ostream& {
            PlaneToCubeMapping mapping = planeToCube(cubeSize, layer(face, cubeSize));
            for (size_t s = 0; s < spaces; s++) os << " ";
            for (size_t x = 0; x < cubeSize; x++) {
                const Piece& p = cube[index(cubeSize, mapping(x, row))];
                piece::print_face(p, face, os) << " ";
            }
            return os;
        };
        
        for (size_t y = 0; y < cubeSize; y++) {
            printRow(cube, os, cubeSize*2, y, Face::Top) << "\n";
        }
        for (size_t y = 0; y < cubeSize; y++) {
            printRow(cube, os, 0, y, Face::Left);
            printRow(cube, os, 0, y, Face::Front);
            printRow(cube, os, 0, y, Face::Right);
            printRow(cube, os, 0, y, Face::Back);
            os << "\n";
        }
        for (size_t y = 0; y < cubeSize; y++) {
            printRow(cube, os, cubeSize*2, y, Face::Bottom) << "\n";
        }
        return os;
    }
}

Cube perform(size_t cubeSize, const std::string& moves)
{
    std::vector<std::string> tokens;
    std::istringstream iss(moves);
    std::copy(std::istream_iterator<std::string>(iss),
              std::istream_iterator<std::string>(),
              std::back_inserter(tokens));
    
    auto performMove = [cubeSize](const char& move, bool clockwise, Cube&& cube) -> Cube {
        switch (move) {
            case 'F':
                return cube::rotate(cubeSize, std::move(cube), layer_front(cubeSize), clockwise);
            case 'U':
                return cube::rotate(cubeSize, std::move(cube), layer_top(cubeSize), clockwise);
            case 'R':
                return cube::rotate(cubeSize, std::move(cube), layer_right(cubeSize), clockwise);
            case 'B':
                return cube::rotate(cubeSize, std::move(cube), layer_back(cubeSize), clockwise);
            case 'L':
                return cube::rotate(cubeSize, std::move(cube), layer_left(cubeSize), clockwise);
            case 'D':
                return cube::rotate(cubeSize, std::move(cube), layer_bottom(cubeSize), clockwise);
            default:
                return std::move(cube);
        }
    };
    
    auto computeMove = [performMove](const std::string& move, Cube&& cube) -> Cube {
        if (move.size() == 1) {
            return performMove(move[0], true, std::move(cube));
        } else {
            if (move[1] == '\'') {
                return performMove(move[0], false, std::move(cube));
            } else if (move[1] == '2') {
                return performMove(move[0], true, performMove(move[0], true, std::move(cube)));
            }
        }
        return std::move(cube);
    };
    
    Cube c = cube::default_cube(cubeSize);
    
    for (const std::string& move : tokens) {
        c = computeMove(move, std::move(c));
    }
    
    return c;
}

std::string random_moves(size_t cubeSize, size_t length)
{
    std::random_device rd;
    auto randomLayer = [cubeSize, &rd](std::stringstream& os) -> std::string {
        if (cubeSize > 3) {
            // TODO
//            std::uniform_int_distribution<int> dist(0, cubeSize);
        }
        std::uniform_int_distribution<int> dist(0, 5);
        int layer = dist(rd);
        switch (layer) {
            case 0:
                return "L";
            case 1:
                return "R";
            case 2:
                return "D";
            case 3:
                return "T";
            case 4:
                return "F";
            case 5:
                return "B";
            default:
                return "";
        }
    };
    
    auto randomTurn = [&rd](std::stringstream& os) -> std::string {
        std::uniform_int_distribution<int> dist(0, 3);
        int turn = dist(rd);
        switch (turn) {
            case 0:
                return "";
            case 1:
                return "'";
            case 2:
                return "2";
            default:
                return "";
        }
    };
    
    std::stringstream ss;
    for (size_t i = 0; i < length; i++) {
        ss << randomLayer(ss) << randomTurn(ss) << " ";
    }
    return ss.str();
}

void printHelp()
{
    std::cerr << "Usage: cube [cube size] [scramble length]" << std::endl;
}

int main(int argc, const char * argv[]) {
    if (argc < 3) {
        printHelp();
        return 1;
    }
    
    size_t cubeSize = std::stoull(std::string(argv[1]));
    size_t scrambleLength = std::stoull(std::string(argv[2]));
    
    std::string scramble_instructions = random_moves(3, scrambleLength);
    std::cout << scramble_instructions << "\n\n";
    Cube c = perform(cubeSize, scramble_instructions);
    cube::print(cubeSize, c, std::cout);
    return 0;
}
