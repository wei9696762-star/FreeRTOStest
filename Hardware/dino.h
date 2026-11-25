#ifndef __DINO_H
#define __DINO_H
void Show_Score(void);
void Show_Ground(void);
void Show_Barrier(void);
void Show_Cloud(void);
void Dino_Tick(void);
void Show_Dino(void);
int DinoGame_Animation(void);
void DinoGame_Pos_Init(void);
void DinoGame_Pos_Init(void);
int isColliding(struct Object_Position *a, struct Object_Position *b);
#endif
