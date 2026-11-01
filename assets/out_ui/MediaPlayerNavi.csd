<GameFile>
  <PropertyGroup Name="MediaPlayerNavi" Type="Layer" ID="165w9ehbmnpvfzt70j32rgl-yukxioq4ds8a" Version="3.10.0.0" />
  <Content ctype="GameProjectContent">
    <Content>
      <Animation Duration="0" Speed="1.000000">
      </Animation>
      <ObjectData Name="Layer" ctype="GameNodeObjectData">
        <Size X="960.000000" Y="640.000000" />
        <Children>
          <AbstractNodeData Name="NaviBar" ActionTag="210411868" Tag="10" RotationSkewX="0" RotationSkewY="0" LeftMargin="0" RightMargin="0" TopMargin="0" BottomMargin="490" TouchEnable="True" HorizontalEdge="BothEdge" VerticalEdge="TopEdge" ComboBoxIndex="1" LeftEage="0" RightEage="0" TopEage="0" BottomEage="0" Scale9OriginX="0" Scale9OriginY="0" Scale9Width="0" Scale9Height="0" ctype="PanelObjectData">
            <Size Y="150" X="960" />
            <AnchorPoint ScaleX="0" ScaleY="0" />
            <Position X="0" Y="490" />
            <Scale ScaleX="1" ScaleY="1" />
            <CColor A="255" B="255" G="255" R="255" />
            <FileData />
            <FirstColor A="255" B="255" G="200" R="150" />
            <EndColor A="255" B="255" G="255" R="255" />
            <ColorVector ScaleX="0" ScaleY="1" />
            <SingleColor A="255" B="42" G="42" R="42" />
            <Children>
              <AbstractNodeData Name="Title" ActionTag="-2067758255" Tag="11" RotationSkewX="0" RotationSkewY="0" LeftMargin="80" RightMargin="80" TopMargin="20" BottomMargin="58" HorizontalEdge="BothEdge" VerticalEdge="TopEdge" FontSize="40" LabelText="Text Label" IsCustomSize="True" OutlineSize="1" ShadowOffsetX="2" ShadowOffsetY="-2" ctype="TextObjectData">
                <Size Y="52" X="800" />
                <AnchorPoint ScaleX="0.5" ScaleY="1" />
                <Position X="480" Y="130" />
                <Scale ScaleX="1" ScaleY="1" />
                <CColor A="255" B="255" G="255" R="255" />
                <FontResource Type="Normal" Path="NotoSansCJK-Regular.ttc" Plist="" />
                <OutlineColor A="255" B="0" G="0" R="255" />
                <ShadowColor A="255" B="110" G="110" R="110" />
              </AbstractNodeData>
              <AbstractNodeData Name="Timeline" ActionTag="-1226225042" Tag="12" RotationSkewX="0" RotationSkewY="0" LeftMargin="20" RightMargin="20" TopMargin="93" BottomMargin="45" TouchEnable="True" HorizontalEdge="BothEdge" VerticalEdge="BottomEdge" ctype="SliderObjectData">
                <Size Y="20" X="920" />
                <AnchorPoint ScaleX="0.5" ScaleY="0.5" />
                <Position X="480" Y="55" />
                <Scale ScaleX="1" ScaleY="1" />
                <CColor A="255" B="255" G="255" R="255" />
                <BackGroundData Type="Default" Path="Default/Slider_Back.png" Plist="" />
                <ProgressBarData Type="Default" Path="Default/Slider_PressBar.png" Plist="" />
                <BallNormalData Type="Default" Path="Default/SliderNode_Normal.png" Plist="" />
                <BallPressedData Type="Default" Path="Default/SliderNode_Press.png" Plist="" />
                <BallDisabledData Type="Default" Path="Default/SliderNode_Disable.png" Plist="" />
              </AbstractNodeData>
              <AbstractNodeData Name="Back" ActionTag="259485318" Tag="37" RotationSkewX="0" RotationSkewY="0" LeftMargin="20" RightMargin="620" TopMargin="0" BottomMargin="20" TouchEnable="True" HorizontalEdge="LeftEdge" VerticalEdge="TopEdge" LeftEage="0" RightEage="0" TopEage="0" BottomEage="0" Scale9OriginX="0" Scale9OriginY="0" Scale9Width="64" Scale9Height="64" ShadowOffsetX="0" ShadowOffsetY="0" ButtonText="" FontSize="18" ctype="ButtonObjectData">
                <Size Y="80" X="80" />
                <AnchorPoint ScaleX="0" ScaleY="0.5" />
                <Position X="20" Y="110" />
                <Scale ScaleX="1" ScaleY="1" />
                <CColor A="255" B="255" G="255" R="255" />
                <NormalFileData Type="Normal" Path="img/back_btn_off.png" Plist="" />
                <PressedFileData Type="Normal" Path="img/back_btn_on.png" Plist="" />
                <DisabledFileData Type="Normal" Path="img/back_btn_on.png" Plist="" />
                <TextColor A="255" B="199" G="199" R="199" />
                <OutlineColor A="255" B="0" G="0" R="255" />
                <FontResource />
                <ShadowColor A="255" B="80" G="127" R="255" />
              </AbstractNodeData>
              <AbstractNodeData Name="PlayTime" ActionTag="1409097770" Tag="39" RotationSkewX="0" RotationSkewY="0" LeftMargin="20" RightMargin="794" TopMargin="103" BottomMargin="5" HorizontalEdge="LeftEdge" VerticalEdge="BottomEdge" FontSize="32" LabelText="Text Label" OutlineSize="1" ShadowOffsetX="2" ShadowOffsetY="-2" ctype="TextObjectData">
                <Size Y="42" X="146" />
                <AnchorPoint ScaleX="0" ScaleY="0" />
                <Position X="20" Y="5" />
                <Scale ScaleX="1" ScaleY="1" />
                <CColor A="255" B="255" G="255" R="255" />
                <FontResource Type="Normal" Path="NotoSansCJK-Regular.ttc" Plist="" />
                <OutlineColor A="255" B="0" G="0" R="255" />
                <ShadowColor A="255" B="110" G="110" R="110" />
              </AbstractNodeData>
              <AbstractNodeData Name="RemainTime" ActionTag="-1091607380" Tag="40" RotationSkewX="0" RotationSkewY="0" LeftMargin="794" RightMargin="20" TopMargin="98" BottomMargin="5" HorizontalEdge="RightEdge" VerticalEdge="BottomEdge" FontSize="32" LabelText="Text Label" OutlineSize="1" ShadowOffsetX="2" ShadowOffsetY="-2" ctype="TextObjectData">
                <Size Y="42" X="146" />
                <AnchorPoint ScaleX="1" ScaleY="0" />
                <Position X="940" Y="5" />
                <Scale ScaleX="1" ScaleY="1" />
                <CColor A="255" B="255" G="255" R="255" />
                <FontResource Type="Normal" Path="NotoSansCJK-Regular.ttc" Plist="" />
                <OutlineColor A="255" B="0" G="0" R="255" />
                <ShadowColor A="255" B="110" G="110" R="110" />
              </AbstractNodeData>
            </Children>
          </AbstractNodeData>
        </Children>
      </ObjectData>
    </Content>
  </Content>
</GameFile>
