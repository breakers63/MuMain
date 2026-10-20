#include "stdafx.h"
#include "NewUIOptionWindow.h"
#include "Data/GameConfig/GameConfig.h"

namespace SEASON3B
{
bool CNewUIOptionWindow::GetReduceParticlesSprites()
{
    return GameConfig::GetInstance().IsVisualReductionEnabled(Data::Graphics::VisualReduction::Particles);
}
bool CNewUIOptionWindow::GetReduceEnvironment()
{
    return GameConfig::GetInstance().IsVisualReductionEnabled(Data::Graphics::VisualReduction::Environment);
}
bool CNewUIOptionWindow::GetReduceBoids()
{
    return GameConfig::GetInstance().IsVisualReductionEnabled(Data::Graphics::VisualReduction::Animals);
}
bool CNewUIOptionWindow::GetReduceShadows()
{
    return GameConfig::GetInstance().IsVisualReductionEnabled(Data::Graphics::VisualReduction::Shadows);
}
}
