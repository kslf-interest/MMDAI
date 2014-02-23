#include "Common.h"

TEST(PMDModelTest, AddAndRemoveBone)
{
    Encoding encoding(0);
    Model model(&encoding);
    std::unique_ptr<IBone> bone(model.createBone());
    ASSERT_EQ(-1, bone->index());
    model.addBone(0); /* should not be crashed */
    model.addBone(bone.get());
    model.addBone(bone.get()); /* no effect because it's already added */
    ASSERT_EQ(1, model.bones().count());
    ASSERT_EQ(bone.get(), model.findBoneRefAt(0));
    ASSERT_EQ(bone->index(), model.findBoneRefAt(0)->index());
    model.removeBone(0); /* should not be crashed */
    model.removeBone(bone.get());
    ASSERT_EQ(0, model.bones().count());
    ASSERT_EQ(-1, bone->index());
    MockIBone mockedBone;
    EXPECT_CALL(mockedBone, index()).WillOnce(Return(-1));
    EXPECT_CALL(mockedBone, name(_)).WillRepeatedly(Return(static_cast<const IString *>(0)));  /* should not be crashed */
    EXPECT_CALL(mockedBone, parentModelRef()).WillOnce(Return(static_cast<IModel *>(0)));
    model.addBone(&mockedBone);
    ASSERT_EQ(0, model.bones().count());
}

TEST_P(PMDLanguageTest, RenameBone)
{
    Encoding encoding(0);
    Model model(&encoding);
    std::unique_ptr<IBone> bone(model.createBone());
    String oldName("OldBoneName"), newName("NewBoneName");
    IEncoding::LanguageType language = GetParam();
    bone->setName(&oldName, language);
    model.addBone(bone.get());
    ASSERT_EQ(bone.get(), model.findBoneRef(&oldName));
    ASSERT_EQ(0, model.findBoneRef(&newName));
    bone->setName(&newName, language);
    ASSERT_EQ(0, model.findBoneRef(&oldName));
    ASSERT_EQ(bone.get(), model.findBoneRef(&newName));
    bone.release();
}
