#include "MaterialEditorDialog.h"
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QListView>
#include <QLineEdit>
#include <QComboBox>
#include <QDialogButtonBox>

MaterialEditorDialog::MaterialEditorDialog(QWidget *parent)
    : QDialog(parent), m_material(nullptr), m_shaderLibrary(nullptr)
{
    // Basic UI layout (replace with .ui file later)
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Material Editor (Qt) - Skeleton"));
    // Add more widgets as needed
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &MaterialEditorDialog::onSaveClicked);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &MaterialEditorDialog::reject);
    layout->addWidget(buttonBox);
}

void MaterialEditorDialog::setMaterial(LibGens::Material* material, LibGens::ShaderLibrary* shaderLibrary)
{
    m_material = material;
    m_shaderLibrary = shaderLibrary;
    // Populate UI fields from material
}

void MaterialEditorDialog::onSaveClicked()
{
    // Save changes to m_material
    accept();
}

void MaterialEditorDialog::onAddTextureUnitClicked()
{
    // Add texture unit logic
}

void MaterialEditorDialog::onDeleteTextureUnitClicked()
{
    // Delete texture unit logic
}
