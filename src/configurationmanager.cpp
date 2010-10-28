#include "configurationmanager.h"
#include "ui_configurationmanager.h"
#include "clipboardmodel.h"
#include <QFile>
#include <QSettings>
#include <QtGui/QDesktopWidget>

inline bool readCssFile(QIODevice &device, QSettings::SettingsMap &map)
{
    map.insert( "css", device.readAll() );
    return true;
}

inline bool writeCssFile(QIODevice &, const QSettings::SettingsMap &)
{
    return true;
}

// singleton
ConfigurationManager* ConfigurationManager::m_Instance = 0;

ConfigurationManager::ConfigurationManager(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigurationManager)
{
    ui->setupUi(this);
    ui->tableCommands->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);

    /* datafile for items */
    // do not use registry in windows
    QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                       QCoreApplication::organizationName(),
                       QCoreApplication::applicationName());
    // ini -> dat
    m_datfilename = settings.fileName();
    m_datfilename.replace( QRegExp("ini$"),QString("dat") );

    // read style sheet from configuration
    readStyleSheet();

    m_keys[Interval] = "interval";
    m_keys[Callback] = "callback";
    m_keys[Formats]  = "formats";
    m_keys[MaxItems] = "maxitems";
    m_keys[Priority] = "priority";
    m_keys[Editor]   = "editor";
    m_keys[ItemHTML] = "format";
    m_keys[CopyClipboard] = "copy_clipboard";
    m_keys[CopySelection] = "copy_selection";
    m_keys[CheckClipboard] = "check_clipboard";
    m_keys[CheckSelection] = "check_selection";

    connect(this, SIGNAL(finished(int)), SLOT(onFinished(int)));

    loadSettings();
}

ConfigurationManager::~ConfigurationManager()
{
    delete ui;
}

QVariant ConfigurationManager::value(Option opt) const
{
    switch(opt) {
    case Interval: return ui->spinInterval->value();
    case Callback: return ui->lineEditScript->text();
    case Formats: return ui->lineEditFormats->text();
    case MaxItems: return ui->spinBoxItems->value();
    case Priority: return ui->lineEditPriority->text();
    case Editor: return ui->lineEditEditor->text();
    case ItemHTML: return ui->plainTextEdit_html->toPlainText();
    case CheckClipboard: return ui->checkBoxClip->isChecked();
    case CheckSelection: return ui->checkBoxSel->isChecked();
    case CopyClipboard: return ui->checkBoxCopyClip->isChecked();
    case CopySelection: return ui->checkBoxCopySel->isChecked();
    default: return QVariant();
    }
}

void ConfigurationManager::loadItems(ClipboardModel &model)
{
    QFile file(m_datfilename);
    file.open(QIODevice::ReadOnly);
    QDataStream in(&file);

    in >> model;
}

void ConfigurationManager::saveItems(const ClipboardModel &model)
{
    QFile file(m_datfilename);
    file.open(QIODevice::WriteOnly);
    QDataStream out(&file);
    out << model;
}

void ConfigurationManager::readStyleSheet()
{
    QSettings::Format cssFormat = QSettings::registerFormat(
            "css", readCssFile, writeCssFile);

    QSettings cssSettings( cssFormat, QSettings::UserScope,
                           QCoreApplication::organizationName(),
                           QCoreApplication::applicationName() );

    QString css = cssSettings.value("css", "").toString();

    setStyleSheet(css);
}

QByteArray ConfigurationManager::windowGeometry(const QString &widget_name, const QByteArray &geometry)
{
    QSettings settings;

    settings.beginGroup("Options");
    if ( geometry.isEmpty() ) {
        return settings.value(widget_name+"_geometry").toByteArray();
    } else {
        settings.setValue(widget_name+"_geometry", geometry);
        return geometry;
    }
    settings.endGroup();
}


void ConfigurationManager::setValue(Option opt, const QVariant &value)
{
    switch(opt) {
    case Interval:
        ui->spinInterval->setValue( value.toInt() );
        break;
    case Callback:
        ui->lineEditScript->setText( value.toString() );
        break;
    case Formats:
        ui->lineEditFormats->setText( value.toString() );
        break;
    case MaxItems:
        ui->spinBoxItems->setValue( value.toInt() );
        break;
    case Priority:
        ui->lineEditPriority->setText( value.toString() );
        break;
    case Editor:
        ui->lineEditEditor->setText( value.toString() );
        break;
    case ItemHTML:
        ui->plainTextEdit_html->setPlainText( value.toString() );
        break;
    case CheckClipboard:
        ui->checkBoxClip->setChecked( value.toBool() );
        break;
    case CheckSelection:
        ui->checkBoxSel->setChecked( value.toBool() );
        break;
    case CopyClipboard:
        ui->checkBoxCopyClip->setChecked( value.toBool() );
        break;
    case CopySelection:
        ui->checkBoxCopySel->setChecked( value.toBool() );
        break;
    default:
        break;
    }
}

void ConfigurationManager::loadSettings()
{
    QSettings settings;

    Option opt;
    QVariant value;

    settings.beginGroup("Options");
    foreach( QString key, settings.allKeys() ) {
        opt = m_keys.key(key);
        value = settings.value(key);
        setValue(opt, value);
    }
    settings.endGroup();

    QTableWidget *table = ui->tableCommands;
    // clear table
    while( table->rowCount()>0 )
        table->removeRow(0);

    int size = settings.beginReadArray("Commands");
    for(int i=0; i<size; ++i)
    {
        settings.setArrayIndex(i);

        int columns = table->columnCount();
        int row = table->rowCount();

        table->insertRow(row);

        for (int col=0; col < columns; ++col) {
            QTableWidgetItem *column = table->horizontalHeaderItem(col);

            QTableWidgetItem *item = new QTableWidgetItem;
            value = settings.value(column->text());
            if( column->text() == tr("Enable") ||
                column->text() == tr("Input") ||
                column->text() == tr("Output") ||
                column->text() == tr("Wait") ) {
                item->setCheckState(value.toBool() ? Qt::Checked : Qt::Unchecked);
            } else {
                if ( value.type() == QVariant::String )
                    item->setText( value.toString() );
                else if ( column->text() == tr("Separator") )
                    item->setText("\\n");
            }
            item->setToolTip( column->toolTip() );
            table->setItem(row, col, item);
        }
    }
    settings.endArray();
    ui->tableCommands->indexWidget( table->model()->index(0,0) );
}

ConfigurationManager::Commands ConfigurationManager::commands() const
{
    Commands cmds;

    QTableWidget *table = ui->tableCommands;
    int columns = table->columnCount();
    int rows = table->rowCount();

    for (int row=0; row < rows; ++row) {
        Command cmd;
        QString name;
        bool enabled = true;
        for (int col=0; col < columns; ++col) {
            QTableWidgetItem *column = table->horizontalHeaderItem(col);
            QTableWidgetItem *item = table->item(row, col);

            if ( column->text() == tr("Enable") ) {
                if ( item->checkState() == Qt::Unchecked ) {
                    enabled = false;
                    break;
                }
            } else if ( column->text() == tr("Name") ) {
                name = item->text();
            } else if ( column->text() == tr("Command") ) {
                cmd.cmd = item->text();
            } else if ( column->text() == tr("Input") ) {
                cmd.input = item->checkState() == Qt::Checked;
            } else if ( column->text() == tr("Output") ) {
                cmd.output = item->checkState() == Qt::Checked;
            } else if ( column->text() == tr("Separator") ) {
                cmd.sep = item->text();
            } else if ( column->text() == tr("Match") ) {
                cmd.re = QRegExp( item->text() );
            } else if ( column->text() == tr("Wait") ) {
                cmd.wait = item->checkState() == Qt::Checked;
            } else if ( column->text() == tr("Icon") ) {
                cmd.icon = QIcon( item->text() );
            } else if ( column->text() == tr("Shortcut") ) {
                cmd.shortcut = item->text();
            }
        }
        if (enabled) {
            cmds[name] = cmd;
        }
    }

    return cmds;
}

void ConfigurationManager::saveSettings()
{
    QSettings settings;

    settings.beginGroup("Options");
    foreach( Option opt, m_keys.keys() ) {
        settings.setValue(m_keys[opt], value(opt));
    }
    settings.endGroup();

    QTableWidget *table = ui->tableCommands;
    int columns = table->columnCount();
    int rows = table->rowCount();

    settings.beginWriteArray("Commands");
    settings.remove("");
    for (int row=0; row < rows; ++row) {
        settings.setArrayIndex(row);
        for (int col=0; col < columns; ++col) {
            QTableWidgetItem *column = table->horizontalHeaderItem(col);
            QTableWidgetItem *item = table->item(row, col);

            if ( column->text() == tr("Enable") ||
                 column->text() == tr("Input") ||
                 column->text() == tr("Output") ||
                 column->text() == tr("Wait") ) {
                settings.setValue( column->text(), item->checkState() == Qt::Checked );
            } else {
                settings.setValue( column->text(), item->text() );
            }
        }
    }
    settings.endArray();
}

void ConfigurationManager::on_buttonBox_clicked(QAbstractButton* button)
{
    switch( ui->buttonBox->buttonRole(button) ) {
    case QDialogButtonBox::ApplyRole:
        accept();
        break;
    case QDialogButtonBox::AcceptRole:
        accept();
        close();
        break;
    case QDialogButtonBox::RejectRole:
        case QDialogButtonBox::ResetRole:
        loadSettings();
        break;
    default:
        return;
    }
}

void ConfigurationManager::addCommand(const QString &name, const Command *cmd, bool enable)
{
    QTableWidget *table = ui->tableCommands;

    int columns = table->columnCount();
    int row = table->rowCount();

    table->insertRow(row);

    for (int col=0; col < columns; ++col) {
        QTableWidgetItem *column = table->horizontalHeaderItem(col);

        QTableWidgetItem *item = new QTableWidgetItem;
        if ( column->text() == tr("Enable") ) {
            item->setCheckState(enable ? Qt::Checked : Qt::Unchecked);
            item->setFlags( item->flags() & ~Qt::ItemIsEditable );
        } else if ( column->text() == tr("Name") ) {
            item->setText(name);
        } else if ( column->text() == tr("Command") ) {
            item->setText(cmd->cmd);
        } else if ( column->text() == tr("Input") ) {
            item->setCheckState(cmd->input ? Qt::Checked : Qt::Unchecked);
        } else if ( column->text() == tr("Output") ) {
            item->setCheckState(cmd->output ? Qt::Checked : Qt::Unchecked);
        } else if ( column->text() == tr("Separator") ) {
            item->setText(cmd->sep);
        } else if ( column->text() == tr("Match") ) {
            item->setText(cmd->re.pattern());
        } else if ( column->text() == tr("Wait") ) {
            item->setCheckState(cmd->wait ? Qt::Checked : Qt::Unchecked);
        } else if ( column->text() == tr("Icon") ) {
            item->setText( cmd->icon.name() );
        } else if ( column->text() == tr("Shortcut") ) {
            item->setText(cmd->shortcut);
        }
        item->setToolTip( column->toolTip() );
        table->setItem(row, col, item);
    }

    saveSettings();
    if (enable) {
        emit configurationChanged();
    }
}

void ConfigurationManager::accept()
{
    setStyleSheet( ui->plainTextEdit_css->toPlainText() );
    emit configurationChanged();
    saveSettings();
}

void ConfigurationManager::on_pushButtoAdd_clicked()
{
    Command cmd;
    cmd.input = cmd.output = cmd.wait = false;
    cmd.sep = QString('\n');
    addCommand(QString(), &cmd);

    QTableWidget *table = ui->tableCommands;
    table->selectRow( table->rowCount()-1 );
}


void ConfigurationManager::on_pushButtonRemove_clicked()
{
    const QItemSelectionModel *sel = ui->tableCommands->selectionModel();

    // remove selected rows
    QModelIndexList rows = sel->selectedRows();
    while ( !rows.isEmpty() ) {
        ui->tableCommands->removeRow( rows.first().row() );
        rows = sel->selectedRows();
    }
}

void ConfigurationManager::on_tableCommands_itemSelectionChanged()
{
    // enable Remove button only if row is selected
    QItemSelectionModel *sel = ui->tableCommands->selectionModel();
    bool enabled = sel->selectedRows().count() != 0;
    ui->pushButtonRemove->setEnabled( enabled );
    ui->pushButtonUp->setEnabled( enabled );
    ui->pushButtonDown->setEnabled( enabled );
}

void ConfigurationManager::showEvent(QShowEvent *e)
{
    QDialog::showEvent(e);

    /* try to resize the dialog so that vertical scrollbar in the about
     * document is hidden
     */
    restoreGeometry( windowGeometry(objectName()) );
}

void ConfigurationManager::onFinished(int)
{
    windowGeometry( objectName(), saveGeometry() );
}

void ConfigurationManager::on_pushButtonUp_clicked()
{
    QTableWidget *table = ui->tableCommands;
    QItemSelectionModel *sel = table->selectionModel();

    int columns = table->columnCount();

    QList<int> rows;
    foreach ( QModelIndex ind, sel->selectedRows() ) {
        rows << ind.row();
    }
    qSort( rows.begin(), rows.end() );

    foreach ( int row, rows ) {
        if (row == 0) break;
        for ( int col=0; col<columns; ++col ) {
            QTableWidgetItem *item1 = table->takeItem(row, col);
            QTableWidgetItem *item2 = table->takeItem(row-1, col);
            table->setItem(row, col, item2);
            table->setItem(row-1, col, item1);
            table->selectionModel()->select( table->model()->index(row, col),
                                             QItemSelectionModel::Deselect );
            table->selectionModel()->select( table->model()->index(row-1, col),
                                             QItemSelectionModel::Select );
        }
    }
}

void ConfigurationManager::on_pushButtonDown_clicked()
{
    QTableWidget *table = ui->tableCommands;
    QItemSelectionModel *sel = table->selectionModel();

    int columns = table->columnCount();

    QList<int> rows;
    foreach ( QModelIndex ind, sel->selectedRows() ) {
        rows << ind.row();
    }
    qSort( rows.begin(), rows.end(), qGreater<int>() );

    foreach ( int row, rows ) {
        if ( row == table->rowCount()-1 ) break;
        for ( int col=0; col<columns; ++col ) {
            QTableWidgetItem *item1 = table->takeItem(row, col);
            QTableWidgetItem *item2 = table->takeItem(row+1, col);
            table->setItem(row, col, item2);
            table->setItem(row+1, col, item1);
            table->selectionModel()->select( table->model()->index(row, col),
                                             QItemSelectionModel::Deselect );
            table->selectionModel()->select( table->model()->index(row+1, col),
                                             QItemSelectionModel::Select );
        }
    }
}