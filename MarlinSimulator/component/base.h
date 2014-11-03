#ifndef SIM_BASE_H
#define SIM_BASE_H

#include <stdint.h>
#include <vector>

class simBaseComponent;
extern std::vector<simBaseComponent*> simComponentList;

class simBaseComponent
{
public:
    simBaseComponent() : drawPosX(-1), drawPosY(-1) {simComponentList.push_back(this);}
    virtual ~simBaseComponent() {}
    
    virtual void tick() {}//Called about every ms
    void doDraw() { if (drawPosX > -1) draw(drawPosX, drawPosY); }
    virtual void draw(int x, int y) {}//Called every screen draw (~25ms)

    simBaseComponent* setDrawPosition(int x, int y) { drawPosX = x; drawPosY = y; return this; }
private:
    int drawPosX, drawPosY;
};

void drawRect(const int x, const int y, const int w, const int h, uint32_t color);
void drawString(const int x, const int y, const char* str, uint32_t color);
void drawChar(const int x, const int y, const char c, uint32_t color);
void drawStringSmall(const int x, const int y, const char* str, uint32_t color);
void drawCharSmall(const int x, const int y, const char c, uint32_t color);

#endif//SIM_BASE_H
