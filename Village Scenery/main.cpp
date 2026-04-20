#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>

int x1 = 5, y1 = 8, x2 = 12, y2 = 3;

void setPixel(int x, int y)
{
    glColor3f(1, 1, 0);
    glBegin(GL_POLYGON);
        glVertex2i(x, y);
        glVertex2i(x + 1, y);
        glVertex2i(x + 1, y + 1);
        glVertex2i(x, y + 1);
    glEnd();
}

void drawGrid()
{
    glColor3f(1, 1, 1);
    for(int i = 0; i <= 15; i++)
    {
        glBegin(GL_LINES);
            glVertex2i(i, 0);
            glVertex2i(i, 15);
            glVertex2i(0, i);
            glVertex2i(15, i);
        glEnd();
    }
}

void bresenham()
{
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = (x2 > x1) ? 1 : -1;
    int sy = (y2 > y1) ? 1 : -1;

    int x = x1;
    int y = y1;

    if (dx >= dy)
    {
        int p = 2 * dy - dx;
        for(int i = 0; i <= dx; i++)
        {
            setPixel(x, y);
            if (p >= 0)
            {
                y += sy;
                p += 2 * (dy - dx);
            }
            else
            {
                p += 2 * dy;
            }
            x += sx;
        }
    }
    else
    {
        int p = 2 * dx - dy;
        for(int i = 0; i <= dy; i++)
        {
            setPixel(x, y);
            if (p >= 0)
            {
                x += sx;
                p += 2 * (dx - dy);
            }
            else
            {
                p += 2 * dx;
            }
            y += sy;
        }
    }
}

void display()
{
    glClear(GL_COLOR_BUFFER_BIT);
    drawGrid();
    bresenham();
    glFlush();
}

void init()
{
    glClearColor(0, 0, 0, 0);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, 15, 0, 15);
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize(600, 600);
    glutCreateWindow("Bresenham Line Drawing");
    init();
    glutDisplayFunc(display);
    glutMainLoop();
    return 0;
}
