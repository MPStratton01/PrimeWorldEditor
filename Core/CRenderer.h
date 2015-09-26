#ifndef CRENDERER_H
#define CRENDERER_H

#include <gl/glew.h>

#include "CCamera.h"
#include "CGraphics.h"
#include "CRenderBucket.h"
#include "ERenderOptions.h"
#include "ERenderCommand.h"
#include "SRenderablePtr.h"
#include <Common/CAABox.h>
#include <Common/CColor.h>
#include <Common/CMatrix4f.h>
#include <Common/types.h>
#include <OpenGL/CFramebuffer.h>
#include <Resource/CFont.h>
#include <Resource/CLight.h>
#include <Resource/CTexture.h>
#include <Scene/CSceneNode.h>

class CRenderer
{
public:
    enum EBloomMode {
        eNoBloom, eBloom, eBloomMaps, eFakeBloom
    };

private:
    ERenderOptions mOptions;
    EBloomMode mBloomMode;
    bool mDrawGrid;
    CColor mClearColor;
    u32 mContextIndex;
    CRenderBucket mOpaqueBucket;
    CRenderBucket mTransparentBucket;
    bool mInitialized;
    u32 mViewportWidth, mViewportHeight;
    u32 mBloomWidth, mBloomHeight;
    float mBloomHScale, mBloomVScale;

    CFramebuffer mSceneFramebuffer;
    CFramebuffer mBloomFramebuffers[3];
    GLint mDefaultFramebuffer;

    // Static Members
    static u32 sNumRenderers;

public:
    // Initialization
    CRenderer();
    ~CRenderer();
    void Init();

    // Getters/Setters
    ERenderOptions RenderOptions();
    void ToggleWorld(bool b);
    void ToggleWorldCollision(bool b);
    void ToggleObjects(bool b);
    void ToggleObjectCollision(bool b);
    void ToggleLights(bool b);
    void ToggleSky(bool b);
    void ToggleBackfaceCull(bool b);
    void ToggleUVAnimation(bool b);
    void ToggleGrid(bool b);
    void ToggleOccluders(bool b);
    void ToggleAlphaDisabled(bool b);
    void SetBloom(EBloomMode BloomMode);
    void SetFont(CFont *pFont);
    void SetClearColor(CColor Clear);
    void SetViewportSize(u32 Width, u32 Height);

    // Render
    void RenderBuckets(CCamera& Camera);
    void RenderBloom();
    void RenderSky(CModel *pSkyboxModel, CCamera& Camera);
    void AddOpaqueMesh(IRenderable *pRenderable, u32 AssetID, CAABox& AABox, ERenderCommand Command);
    void AddTransparentMesh(IRenderable *pRenderable, u32 AssetID, CAABox& AABox, ERenderCommand Command);
    void BeginFrame();
    void EndFrame();
    void ClearDepthBuffer();

    // Private
private:
    void InitFramebuffer();
};

extern u32 gDrawCount;

#endif // RENDERMANAGER_H
