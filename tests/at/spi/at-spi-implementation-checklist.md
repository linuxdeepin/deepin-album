# deepin-album AT-SPI Implementation Checklist

## Gap Summary

| Gap ID | File | Class | Line | Action | Priority |
|--------|------|-------|------|--------|----------|
| G1 | src/src/widgets/timelineview/timelineview.cpp | TimeLineView (DWidget) | ~48 | Add setObjectName + setAccessibleName | Required |
| G2 | src/src/widgets/importtimelineview/importtimelineview.cpp | ImportTimeLineView (DWidget) | ~35 | Add setObjectName + setAccessibleName | Required |
| G3 | src/src/widgets/thumbnail/thumbnaillistview.cpp | ThumbnailListView (DListView) | ~88 | Add setObjectName + setAccessibleName | Required |
| G4 | src/src/widgets/widgtes/noresultwidget.cpp | NoResultWidget (QWidget) | ~12 | Add setObjectName + setAccessibleName | Required |
| G5 | src/src/widgets/widgtes/expansionmenu.cpp | FilterWidget (QWidget) | ~28 | Add setObjectName + setAccessibleName | Required |
| G6 | src/src/widgets/widgtes/expansionmenu.cpp | FilterLabel (QLabel) | ~14 | Add setObjectName + setAccessibleName | Required |
| G7 | src/src/widgets/widgtes/toolbutton.cpp | ToolButton (DPushButton) | ~12 | Add setObjectName + setAccessibleName | Required |
| G8 | src/src/widgets/widgtes/expansionpanel.cpp | ExpansionPanel (DBlurEffectWidget) | ~30 | Add setObjectName + setAccessibleName | Required |
| G9 | src/src/widgets/thumbnail/timelinedatewidget.cpp | TimeLineDateWidget (DWidget) | ~17 | Add setObjectName + setAccessibleName | Required |
| G10 | src/src/widgets/thumbnail/timelinedatewidget.cpp | importTimeLineDateWidget (DWidget) | ~150 | Add setObjectName + setAccessibleName | Required |

## Implementation Notes

Each gap requires adding the following lines **immediately after the constructor
body begins**, before any child widget creation:

```cpp
setObjectName("ClassName");
setAccessibleName("ClassName");
```

For QWidget subclasses both calls are required.  
Not needed for: QQuickPaintedItem (QQuickItem subclasses — use QML Accessible),
QObject subclasses, data-only classes.

## Validation

- Build with cmake + make (or ninja) — must compile without errors
- libclang AST scan can verify coverage improvement from 0% to ~90%