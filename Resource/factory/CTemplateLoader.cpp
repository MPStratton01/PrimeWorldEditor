#include "CTemplateLoader.h"
#include "CWorldLoader.h"
#include <Core/Log.h>

void CTemplateLoader::LoadStructProperties(tinyxml2::XMLElement *pElem, CStructTemplate *pTemp, const std::string& templateName)
{
    tinyxml2::XMLElement *pChild = pElem->FirstChildElement();

    while (pChild)
    {
        CPropertyTemplate *pProp = LoadPropertyTemplate(pChild, templateName);

        if (pProp)
            pTemp->mProperties.push_back(pProp);

        pChild = pChild->NextSiblingElement();
    }
}

CPropertyTemplate* CTemplateLoader::LoadPropertyTemplate(tinyxml2::XMLElement *pElem, const std::string& templateName)
{
    const char *kpIDStr = pElem->Attribute("ID");
    const char *kpNameStr = pElem->Attribute("name");
    const char *kpTypeStr = pElem->Attribute("type");
    const char *kpExtensionsStr = pElem->Attribute("ext");
    const char *kpTemplateStr = pElem->Attribute("template");

    // Get ID + name, find source template if it exists
    u32 ID = StringUtil::ToInt32(kpIDStr);
    CPropertyTemplate *pSource = nullptr;
    std::string name;

    if (mpMaster->HasPropertyList())
        pSource = mpMaster->GetProperty(ID);

    if (kpNameStr)
        name = kpNameStr;
    else if (pSource)
        name = pSource->Name();
    else
        name = StringUtil::ToHexString(ID);

    // Load Property
    if (strcmp(pElem->Name(), "property") == 0)
    {
        CPropertyTemplate *pProp;
        EPropertyType type = eInvalidProperty;

        // Type
        if (kpTypeStr)
            type = PropStringToPropEnum(kpTypeStr);
        else if (pSource)
            type = pSource->Type();

        // File property
        if (type == eFileProperty)
        {
            CStringList extensions;
            if (kpExtensionsStr)
                extensions = StringUtil::Tokenize(kpExtensionsStr, ",");
            else if (pSource)
                extensions = static_cast<CFileTemplate*>(pSource)->Extensions();

            pProp = new CFileTemplate(name, ID, extensions);
        }

        // Regular property
        else
            pProp = new CPropertyTemplate(type, name, ID);

        return pProp;
    }

    // Load Struct
    else if (strcmp(pElem->Name(), "struct") == 0)
    {
        CStructTemplate *pStruct = new CStructTemplate();

        pStruct->mPropID = ID;

        // Read children properties
        // Priority: [Embedded] -> [Template] -> [Master]

        // Embedded
        if (!pElem->NoChildren())
            LoadStructProperties(pElem, pStruct, templateName);

        // Template
        else if (kpTemplateStr)
        {
            std::string tempPath = mMasterDir + kpTemplateStr;

            tinyxml2::XMLDocument structXML;
            structXML.LoadFile(tempPath.c_str());

            if (structXML.Error())
                Log::Error("Couldn't open struct XML: " + mMasterDir + kpTemplateStr);

            else
            {
                tinyxml2::XMLElement *pRoot = structXML.FirstChildElement("struct");
                pStruct->mSourceFile = kpTemplateStr;

                if (pRoot->Attribute("type"))
                    pStruct->mIsSingleProperty = (strcmp(pRoot->Attribute("type"), "single") == 0);

                if (pRoot->Attribute("name"))
                    pStruct->mPropName = pRoot->Attribute("name");

                LoadStructProperties(pRoot, pStruct, templateName);
            }
        }

        // Master
        else if (pSource)
        {
            CStructTemplate *pSourceStruct = static_cast<CStructTemplate*>(pSource);

            for (u32 iProp = 0; iProp < pSourceStruct->Count(); iProp++)
                pStruct->mProperties.push_back(pSourceStruct->PropertyByIndex(iProp));
        }

        // If it's none of these, then it probably has no children because it's a property list entry.

        // Single property?
        if (kpTypeStr)
            pStruct->mIsSingleProperty = (strcmp(kpTypeStr, "single") == 0);
        else if (pSource)
            pStruct->mIsSingleProperty = static_cast<CStructTemplate*>(pSource)->IsSingleProperty();

        // Name
        if (!name.empty())
            pStruct->mPropName = name;

        return pStruct;
    }

    return nullptr;
}

// ************ SCRIPT OBJECT ************
CScriptTemplate* CTemplateLoader::LoadScriptTemplate(tinyxml2::XMLDocument *pDoc, const std::string& templateName, u32 objectID)
{
    CScriptTemplate *pScript = new CScriptTemplate(mpMaster);
    pScript->mObjectID = objectID;

    tinyxml2::XMLElement *pRoot = pDoc->FirstChildElement("ScriptTemplate");

    // Name
    tinyxml2::XMLElement *pNameElem = pRoot->FirstChildElement("name");

    if (pNameElem)
        pScript->mTemplateName = pNameElem->GetText();

    // Properties
    tinyxml2::XMLElement *pPropsElem = pRoot->FirstChildElement("properties");

    while (pPropsElem)
    {
        CScriptTemplate::SPropertySet set;

        const char *kpVersion = pPropsElem->Attribute("version");
        set.SetName = (kpVersion ? kpVersion : "");
        set.pBaseStruct = new CStructTemplate();
        set.pBaseStruct->mIsSingleProperty = false;
        set.pBaseStruct->mPropID = -1;
        set.pBaseStruct->mPropName = pScript->mTemplateName;
        LoadStructProperties(pPropsElem, set.pBaseStruct, pScript->mTemplateName);
        pScript->mPropertySets.push_back(set);

        pPropsElem = pPropsElem->NextSiblingElement("properties");
    }

    // Editor Parameters
    tinyxml2::XMLElement *pEditor = pRoot->FirstChildElement("editor");

    if (pEditor)
    {
        // Editor Properties
        tinyxml2::XMLElement *pEdProperties = pEditor->FirstChildElement("properties");
        tinyxml2::XMLElement *pEdProp = pEdProperties->FirstChildElement("property");

        while (pEdProp)
        {
            const char *kpName = pEdProp->Attribute("name");
            const char *kpID = pEdProp->Attribute("ID");

            if (kpName && kpID)
            {
                if (strcmp(kpName, "InstanceName") == 0)
                    pScript->mNameIDString = kpID;
                else if (strcmp(kpName, "Position") == 0)
                    pScript->mPositionIDString = kpID;
                else if (strcmp(kpName, "Rotation") == 0)
                    pScript->mRotationIDString = kpID;
                else if (strcmp(kpName, "Scale") == 0)
                    pScript->mScaleIDString = kpID;
                else if (strcmp(kpName, "Active") == 0)
                    pScript->mActiveIDString = kpID;
                else if (strcmp(kpName, "LightParameters") == 0)
                    pScript->mLightParametersIDString = kpID;
            }

            pEdProp = pEdProp->NextSiblingElement("property");
        }

        // Editor Assets
        tinyxml2::XMLElement *pEdAssets = pEditor->FirstChildElement("assets");
        tinyxml2::XMLElement *pAsset = pEdAssets->FirstChildElement();

        while (pAsset)
        {
            const char *kpSource = pAsset->Attribute("source");
            const char *kpID = pAsset->GetText();

            if (kpSource && kpID)
            {
                CScriptTemplate::SEditorAsset asset;

                if (strcmp(pAsset->Name(), "animparams") == 0)
                    asset.AssetType = CScriptTemplate::SEditorAsset::eAnimParams;
                else if (strcmp(pAsset->Name(), "model") == 0)
                    asset.AssetType = CScriptTemplate::SEditorAsset::eModel;
                else if (strcmp(pAsset->Name(), "collision") == 0)
                    asset.AssetType = CScriptTemplate::SEditorAsset::eCollision;
                else
                {
                    pAsset = pAsset->NextSiblingElement();
                    continue;
                }

                if (strcmp(kpSource, "property") == 0)
                    asset.AssetSource = CScriptTemplate::SEditorAsset::eProperty;
                else if (strcmp(kpSource, "file") == 0)
                    asset.AssetSource = CScriptTemplate::SEditorAsset::eFile;
                else
                {
                    pAsset = pAsset->NextSiblingElement();
                    continue;
                }

                const char *kpForce = pAsset->Attribute("force");
                if (kpForce)
                    asset.ForceNodeIndex = StringUtil::ToInt32(kpForce);
                else
                    asset.ForceNodeIndex = -1;

                asset.AssetLocation = kpID;
                pScript->mAssets.push_back(asset);
            }

            pAsset = pAsset->NextSiblingElement();
        }

        // Rotation
        tinyxml2::XMLElement *pRotType = pEditor->FirstChildElement("rotation_type");

        if (pRotType)
        {
            const char *kpType = pRotType->GetText();

            if (kpType)
            {
                if (strcmp(kpType, "disabled") == 0) pScript->mRotationType = CScriptTemplate::eRotationDisabled;
                else pScript->mRotationType = CScriptTemplate::eRotationEnabled;
            }
        }

        // Scale
        tinyxml2::XMLElement *pScaleType = pEditor->FirstChildElement("scale_type");

        if (pScaleType)
        {
            const char *kpType = pScaleType->GetText();

            if (kpType)
            {
                if (strcmp(kpType, "disabled") == 0) pScript->mScaleType = CScriptTemplate::eScaleDisabled;
                else if (strcmp(kpType, "volume") == 0) pScript->mScaleType = CScriptTemplate::eScaleVolume;
                else pScript->mScaleType = CScriptTemplate::eScaleEnabled;
            }
        }

        // Preview Volume
        if (pScript->mScaleType == CScriptTemplate::eScaleVolume)
        {
            tinyxml2::XMLElement *pVolume = pEditor->FirstChildElement("preview_volume");

            // Lambda to avoid duplicating volume shape code
            auto GetVolumeType = [](const char *kpType) -> EVolumeShape {
                if (strcmp(kpType, "none") == 0)           return eNoShape;
                if (strcmp(kpType, "Box") == 0)            return eBoxShape;
                if (strcmp(kpType, "AxisAlignedBox") == 0) return eAxisAlignedBoxShape;
                if (strcmp(kpType, "Ellipsoid") == 0)      return eEllipsoidShape;
                if (strcmp(kpType, "Cylinder") == 0)       return eCylinderShape;
                if (strcmp(kpType, "CylinderLarge") == 0)  return eCylinderLargeShape;
                if (strcmp(kpType, "Conditional") == 0)    return eConditionalShape;
                return eInvalidShape;
            };

            const char *kpShape = pVolume->Attribute("shape");

            if (kpShape)
                pScript->mVolumeShape = GetVolumeType(kpShape);

            // Conditional
            if (pScript->mVolumeShape == eConditionalShape)
            {
                const char *kpID = pVolume->Attribute("propertyID");

                if (kpID)
                {
                    pScript->mVolumeConditionIDString = kpID;
                    tinyxml2::XMLElement *pCondition = pVolume->FirstChildElement("condition");

                    while (pCondition)
                    {
                        const char *kpConditionValue = pCondition->Attribute("value");
                        const char *kpConditionShape = pCondition->Attribute("shape");

                        if (kpConditionValue && kpConditionShape)
                        {
                            CScriptTemplate::SVolumeCondition condition;
                            condition.Shape = GetVolumeType(kpConditionShape);

                            if (strcmp(kpConditionValue, "true") == 0)
                                condition.Value = 1;
                            else if (strcmp(kpConditionValue, "false") == 0)
                                condition.Value = 0;
                            else
                                condition.Value = StringUtil::ToInt32(kpConditionValue);

                            pScript->mVolumeConditions.push_back(condition);
                        }

                        pCondition = pCondition->NextSiblingElement("condition");
                    }
                }
            }
        }
    }

    return pScript;
}

// ************ MASTER ************
void CTemplateLoader::LoadMasterTemplate(tinyxml2::XMLDocument *pDoc)
{
    tinyxml2::XMLElement *pRoot = pDoc->FirstChildElement("MasterTemplate");
    mpMaster->mVersion = StringUtil::ToInt32(pRoot->Attribute("version"));

    tinyxml2::XMLElement *pElem = pRoot->FirstChildElement();

    while (pElem)
    {
        // Properties
        if (strcmp(pElem->Name(), "properties") == 0)
        {
            std::string propListPath = mMasterDir + pElem->GetText();

            tinyxml2::XMLDocument propListXML;
            propListXML.LoadFile(propListPath.c_str());

            if (propListXML.Error())
                Log::Error("Couldn't open property list: " + propListPath);

            else
                LoadPropertyList(&propListXML, propListPath);
        }

        // Objects
        else if (strcmp(pElem->Name(), "objects") == 0)
        {
            tinyxml2::XMLElement *pObj = pElem->FirstChildElement("object");

            while (pObj)
            {
                // ID can either be a hex number or an ASCII fourCC
                std::string strID = pObj->Attribute("ID");
                u32 ID;

                if (StringUtil::IsHexString(strID, true))
                    ID = StringUtil::ToInt32(strID);
                else
                    ID = CFourCC(strID).ToLong();

                // Load up the object
                std::string templateName = pObj->Attribute("template");
                std::string templatePath = mMasterDir + templateName;

                tinyxml2::XMLDocument scriptXML;
                scriptXML.LoadFile(templatePath.c_str());

                if (scriptXML.Error())
                    Log::Error("Couldn't open script template: " + templatePath);

                else
                {
                    CScriptTemplate *pTemp = LoadScriptTemplate(&scriptXML, templateName, ID);

                    if (pTemp)
                    {
                        pTemp->mSourceFile = templateName;
                        mpMaster->mTemplates[ID] = pTemp;
                    }
                }

                pObj = pObj->NextSiblingElement("object");
            }
        }

        // States
        else if (strcmp(pElem->Name(), "states") == 0)
        {
            tinyxml2::XMLElement *pState = pElem->FirstChildElement("state");

            while (pState)
            {
                std::string strID = pState->Attribute("ID");
                u32 stateID;

                if (StringUtil::IsHexString(strID, true))
                    stateID = StringUtil::ToInt32(strID);
                else
                    stateID = CFourCC(strID).ToLong();

                std::string stateName = pState->Attribute("name");
                mpMaster->mStates[stateID] = stateName;
                pState = pState->NextSiblingElement("state");
            }
        }

        // Messages
        else if (strcmp(pElem->Name(), "messages") == 0)
        {
            tinyxml2::XMLElement *pMessage = pElem->FirstChildElement("message");

            while (pMessage)
            {
                std::string strID = pMessage->Attribute("ID");
                u32 messageID;

                if (StringUtil::IsHexString(strID, true))
                    messageID = StringUtil::ToInt32(strID);
                else
                    messageID = CFourCC(strID).ToLong();

                std::string messageName = pMessage->Attribute("name");
                mpMaster->mMessages[messageID] = messageName;
                pMessage = pMessage->NextSiblingElement("message");
            }
        }

        pElem = pElem->NextSiblingElement();
    }
}

void CTemplateLoader::LoadPropertyList(tinyxml2::XMLDocument *pDoc, const std::string& listName)
{
    tinyxml2::XMLElement *pElem = pDoc->FirstChildElement()->FirstChildElement();

    while (pElem)
    {
        CPropertyTemplate *pProp = LoadPropertyTemplate(pElem, listName);

        if (pProp)
            mpMaster->mPropertyList[pProp->PropertyID()] = pProp;

        pElem = pElem->NextSiblingElement();
    }

    mpMaster->mHasPropList = true;
}

CMasterTemplate* CTemplateLoader::LoadGame(tinyxml2::XMLNode *pNode)
{
    tinyxml2::XMLElement *pGameElem = pNode->FirstChildElement();
    mpMaster = new CMasterTemplate();

    // Parse game parameters
    while (pGameElem)
    {
        if (strcmp(pGameElem->Name(), "name") == 0)
            mpMaster->mGameName = pGameElem->GetText();

        else if (strcmp(pGameElem->Name(), "mlvl") == 0)
        {
            u32 VersionNum = std::stoul(pGameElem->GetText(), 0, 16);
            mpMaster->mGame = CWorldLoader::GetFormatVersion(VersionNum);
        }

        else if (strcmp(pGameElem->Name(), "master") == 0)
        {
            std::string MasterPath = mTemplatesDir + pGameElem->GetText();
            mMasterDir = StringUtil::GetFileDirectory(MasterPath);

            tinyxml2::XMLDocument MasterXML;
            MasterXML.LoadFile(MasterPath.c_str());

            if (MasterXML.Error())
            {
                Log::Error("Couldn't open master template at " + MasterPath + " - error " + std::to_string(MasterXML.ErrorID()));
            }

            else
            {
                LoadMasterTemplate(&MasterXML);
                mpMaster->mSourceFile = pGameElem->GetText();
            }
        }
        pGameElem = pGameElem->NextSiblingElement();
    }

    mpMaster->mFullyLoaded = true;
    return mpMaster;
}

// ************ PUBLIC ************
void CTemplateLoader::LoadGameList()
{
    static const std::string skTemplatesDir = "../templates/";
    static const std::string skGameListPath = skTemplatesDir + "GameList.xml";
    Log::Write("Loading game list");

    // Load Game List XML
    tinyxml2::XMLDocument GameListXML;
    GameListXML.LoadFile(skGameListPath.c_str());

    if (GameListXML.Error())
    {
        Log::Error("Couldn't open game list at " + skGameListPath + " - error " + std::to_string(GameListXML.ErrorID()));
        return;
    }

    // Parse
    tinyxml2::XMLNode *pNode = GameListXML.FirstChild()->NextSibling()->FirstChild();

    while (pNode)
    {
        tinyxml2::XMLElement *pElement = pNode->ToElement();

        // Game List version number
        if (strcmp(pElement->Name(), "version") == 0)
        {
            u32 VersionNum = std::stoul(pElement->GetText());
            CMasterTemplate::smGameListVersion = VersionNum;
        }

        // Games
        else if (strcmp(pElement->Name(), "game") == 0)
        {
            CTemplateLoader Loader(skTemplatesDir);
            CMasterTemplate *pMaster = Loader.LoadGame(pNode);

            if (!pMaster->IsLoadedSuccessfully())
            {
                Log::Error("Master template for " + pMaster->mGameName + " couldn't be loaded");
                delete pMaster;
            }

            else
                CMasterTemplate::smMasterMap[pMaster->mGame] = pMaster;
        }

        pNode = pNode->NextSibling();
    }
}
