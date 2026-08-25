#ifndef UI_CONTEXT_H
#define UI_CONTEXT_H

#include "AppOptions.h"
#include "CardFormat.h"
#include "GameView.h"
#include "Renderer.h"
#include "Terminal.h"

// What the human's move provider needs to draw a screen.
//
// IMoveProvider hands an implementation nothing but a hand, a trump and a lead
// suit - it has no route back to the game - so the provider is given this when
// it is constructed instead.
//
// The view is mutable because the provider is what knows the human's hand and
// which cards are playable; it fills those in before drawing.
//
// Everything referenced here outlives the provider only because
// TerminalRomanianWhist declares the engine last. See the note there.
struct UiContext
{
    const AppOptions& options;
    const Terminal& terminal;
    const Renderer& renderer;
    const CardFormat& cardFormat;
    GameView& view;
};

#endif
